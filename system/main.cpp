#include "global.h"
#include "ycsb.h"
#include "tpcc.h"
#include "test.h"
#include "thread.h"
#include "manager.h"
#include "mem_alloc.h"
#include "query.h"
#include "plock.h"
#include "occ.h"
#include "vll.h"
#include "basic_sched.h"

void * f(void *);

thread_t ** m_thds;

// defined in parser.cpp
void parser(int argc, char * argv[]);

int main(int argc, char* argv[])
{
	parser(argc, argv);

	mem_allocator.init(g_part_cnt, MEM_SIZE / g_part_cnt);
	stats.init();
	glob_manager = (Manager *) _mm_malloc(sizeof(Manager), 64);
	glob_manager->init();
	if (g_cc_alg == DL_DETECT)
		dl_detector.init();
	printf("mem_allocator initialized!\n");

#if CC_ALG == WOUND_WAIT
	printf("WOUND_WAIT\n");
#elif CC_ALG == NO_WAIT
	printf("NO_WAIT\n");
#elif CC_ALG == WAIT_DIE
	printf("WAIT_DIE\n");
#elif CC_ALG == BAMBOO
	printf("BAMBOO\n");
#elif CC_ALG == SILO
	printf("SILO\n");
#elif CC_ALG == IC3
	printf("IC3\n");
#elif CC_ALG == ORDERED_LOCK
	printf("ORDERED_LOCK\n");
#elif CC_ALG == BASIC_SCHED
	printf("BASIC_SCHED\n");
#elif CC_ALG == QCC
	printf("QCC\n");
    if (g_part_cnt != 1) {
        printf("qcc does not support multiple partitions\n");
        fflush(stdout);
        exit(1);
    }
#endif

	workload * m_wl;
	switch (WORKLOAD) {
		case YCSB :
			m_wl = new ycsb_wl; break;
		case TPCC :
			m_wl = new tpcc_wl; break;
		case TEST :
			m_wl = new TestWorkload;
			((TestWorkload *)m_wl)->tick();
			break;
		default:
			assert(false);
	}
	m_wl->init();
	printf("workload initialized!\n");

	uint64_t thd_cnt = g_thread_cnt;
	pthread_t p_thds[thd_cnt];
	m_thds = new thread_t * [thd_cnt];
    for (uint32_t i = 0; i < thd_cnt; i++) {
		m_thds[i] = (thread_t *) _mm_malloc(sizeof(thread_t), 64);
        m_thds[i]->txn_commit_latency.reset();
    }
	// query_queue should be the last one to be initialized!!!
	// because it collects txn latency
	query_queue = (Query_queue *) _mm_malloc(sizeof(Query_queue), 64);
	if (WORKLOAD != TEST)
		query_queue->init(m_wl);
	// pthread_barrier_init( &warmup_bar, NULL, g_thread_cnt );
	pthread_barrier_init( &start_bar, NULL, g_thread_cnt+1 );
	printf("query_queue initialized!\n");
#if CC_ALG == HSTORE
	part_lock_man.init();
#elif CC_ALG == OCC
	occ_man.init();
#elif CC_ALG == VLL
	vll_man.init();
#endif

	for (uint32_t i = 0; i < thd_cnt; i++)
		m_thds[i]->init(i, m_wl);

#if CC_ALG == QCC
    u64 nr_workers = g_thread_cnt;
#if WORKLOAD == YCSB || WORKLOAD == TPCC
    struct qcc *const q = qcc_create(nr_workers);
    m_wl->q = q;
    printf("qcc init (m_wl->q = %p) nr_workers %lu\n", q, nr_workers);
#else
    assert(false);
#endif
#endif

#if CC_ALG == BASIC_SCHED
    u64 nr_workers = g_thread_cnt;
#if WORKLOAD == YCSB || WORKLOAD == TPCC
    pthread_t sched_thd;
    struct basic_sched *const s = basic_sched_create();
    m_wl->s = s;
    pthread_create(&sched_thd, NULL, basic_sched_run, (void *)s);
    printf("basic_sched init (m_wl->s = %p)\n", s);
#else
    assert(false);
#endif
#endif

	// if (WARMUP > 0){
	// 	printf("WARMUP start!\n");
	// 	for (uint32_t i = 0; i < thd_cnt - 1; i++) {
	// 		uint64_t vid = i;
	// 		pthread_create(&p_thds[i], NULL, f, (void *)vid);
	// 	}
	// 	f((void *)(thd_cnt - 1));
	// 	for (uint32_t i = 0; i < thd_cnt - 1; i++)
	// 		pthread_join(p_thds[i], NULL);
	// 	printf("WARMUP finished!\n");
	// }
	warmup_finish = true;
	// pthread_barrier_init( &warmup_bar, NULL, g_thread_cnt );
#ifndef NOGRAPHITE
	CarbonBarrierInit(&enable_barrier, g_thread_cnt);
#endif
	// pthread_barrier_init( &warmup_bar, NULL, g_thread_cnt );


	// spawn and run txns again.
	for (uint32_t i = 0; i < thd_cnt; i++) {
		uint64_t vid = i;
		pthread_create(&p_thds[i], NULL, f, (void *)vid);
	}

	pthread_barrier_wait( &start_bar );
	uint64_t starttime = get_server_clock();
    for (uint32_t i = 0; i < thd_cnt; i++) {
		pthread_join(p_thds[i], NULL);
    }

#if CC_ALG == BASIC_SCHED
    s->valid = 0;
    pthread_join(sched_thd, NULL);
#endif
	uint64_t endtime = get_server_clock();

	if (WORKLOAD != TEST) {
		printf("PASS! SimTime = %ld\n", endtime - starttime);
		if (STATS_ENABLE)
			stats.print(endtime-starttime);
	} else {
		((TestWorkload *)m_wl)->summarize();
	}

	Stat_latency txn_commit_latency;

	for (uint32_t i = 0; i < thd_cnt; i++) {
		txn_commit_latency += m_thds[i]->txn_commit_latency;
	}

	printf("[summary!] txn commit latency (us): min=%lu, max=%lu, avg=%lu; 50-th=%lu, 95-th=%lu, 99-th=%lu, 99.9-th=%lu\n",
         txn_commit_latency.min(), txn_commit_latency.max(),
         txn_commit_latency.avg(), txn_commit_latency.perc(0.50),
         txn_commit_latency.perc(0.95), txn_commit_latency.perc(0.99),
         txn_commit_latency.perc(0.999));

#if CC_ALG == QCC
    qcc_destroy(q);
#endif

	return 0;
}

void * f(void * id) {
	uint64_t tid = (uint64_t)id;
	m_thds[tid]->run();
	return NULL;
}
