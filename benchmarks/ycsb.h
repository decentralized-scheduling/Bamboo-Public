#ifndef _SYNTH_BM_H_
#define _SYNTH_BM_H_

#include "wl.h"
#include "txn.h"
#include "global.h"
#include "helper.h"

class ycsb_query;

class ycsb_wl : public workload {
public :
	RC init();
	RC init_table();
	RC init_schema(string schema_file);
	RC get_txn_man(txn_man *& txn_manager, thread_t * h_thd);
	int key_to_part(uint64_t key);
	INDEX * the_index;
	table_t * the_table;
#if CC_ALG == IC3
	SC_PIECE * get_cedges(TPCCTxnType type, int idx) {return NULL;};
#endif
private:
	void init_table_parallel();
	void * init_table_slice();
	static void * threadInitTable(void * This) {
		((ycsb_wl *)This)->init_table_slice();
		return NULL;
	}
	pthread_mutex_t insert_lock;
	//  For parallel initialization
	static int next_tid;
};

class ycsb_txn_man : public txn_man
{
public:
	void init(thread_t * h_thd, workload * h_wl, uint64_t part_id);
	RC run_txn(base_query * query);
	RC run_txn_r(base_query * query);
private:
#if CC_ALG != BAMBOO
	uint64_t row_cnt;
#endif
	ycsb_wl * _wl;

#if CC_ALG == ORDERED_LOCK
    void ol_prepare_ycsb(const ycsb_query *const query);
    void ol_finish_ycsb();
#endif

#if CC_ALG == BASIC_SCHED
    void bs_prepare_ycsb(const ycsb_query *const query);
    void bs_finish_ycsb();
#endif

#if CC_ALG == QCC
    struct qcc_txn *qcc_prepare_ycsb(const ycsb_query *const query);
#endif
};

#endif
