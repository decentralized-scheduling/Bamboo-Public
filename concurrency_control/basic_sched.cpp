#include "basic_sched.h"

#if CC_ALG == BASIC_SCHED

struct basic_sched *basic_sched_create()
{
    struct basic_sched *const s = (struct basic_sched *)malloc(sizeof(*s));
    memset(s, 0, sizeof(*s));
    return s;
}

// static void basic_sched_print_stat(const struct basic_sched *const s, const u64 tid, const char *const msg)
// {
//     const struct basic_sched_request *const r = s->worker_meta[tid].request;
//     if (msg) {
//         printf("basic_sched_print_stat: %s\n", msg);
//     } else {
//         printf("basic_sched_print_stat\n");
//     }
//     for (u64 i=0; i<r->nr_requests; i++) {
//         const u64 id = r->requests[i].id;
//         const access_t type = r->requests[i].type;
//         const u8 wr = (type == WR) ? 1 : 0;
//         printf("%lu %u r %u w %u req %u\n", id, wr,
//                 s->status[id].nr_readers, s->status[id].nr_writers, s->status[id].write_requested);
//     }
//     fflush(stdout);
// }

static u8 basic_sched_schedule(struct basic_sched *const s, const u64 tid)
{
    // first check if all items are available..
    const struct basic_sched_request *const r = s->worker_meta[tid].request;
    for (u64 i=0; i<r->nr_requests; i++) {
        const u64 id = r->requests[i].id;
        const access_t type = r->requests[i].type;
        if (type == WR) {
            if (s->status[id].nr_readers > 0 || s->status[id].nr_writers > 0) {
                if (s->status[id].write_requested == 0) {
                    // block future readers..
                    s->status[id].write_requested = 1;
                }
                return 0;
            }
        } else if (type == RD){
            if (s->status[id].nr_writers > 0) {
                return 0;
            }
        } else {
            printf("basic_sched: request type not WR or RD\n");
            fflush(stdout);
            exit(1);
        }
    }

    // now it can run.. add its meta and release
    for (u64 i=0; i<r->nr_requests; i++) {
        const u64 id = r->requests[i].id;
        const access_t type = r->requests[i].type;
        if (type == WR) {
            s->status[id].write_requested = 0;
            s->status[id].nr_writers++;
        } else if (type == RD){
            s->status[id].nr_readers++;
        } else {
            printf("basic_sched: request type not WR or RD\n");
            fflush(stdout);
            exit(1);
        }
    }
    return 1;
}

static void basic_sched_cleanup(struct basic_sched *const s, const u64 tid)
{
    const struct basic_sched_request *const r = s->worker_meta[tid].request;
    for (u64 i=0; i<r->nr_requests; i++) {
        const u64 id = r->requests[i].id;
        const access_t type = r->requests[i].type;
        if (type == WR) {
            s->status[id].nr_writers--;
        } else if (type == RD){
            s->status[id].nr_readers--;
        } else {
            printf("basic_sched: request type not WR or RD\n");
            fflush(stdout);
            exit(1);
        }
    }
}

void *basic_sched_run(void *const sptr)
{
	set_affinity(0);
    printf("basic_sched scheduler starts running\n");
    fflush(stdout);

    struct basic_sched *const s = (struct basic_sched *const)(sptr);
    s->valid= 1;
    // now let's just do a basic one without using per-object status
    while (s->valid == 1) {
        for (u64 i=0; i<THREAD_CNT; i++) {
            if (s->worker_meta[i].request != NULL) {
                if (s->worker_meta[i].signal == BASIC_SCHED_SIGNAL_WAIT) {
                    // schedule!
                    const u8 ok = basic_sched_schedule(s, i);
                    // now request is not null, and signal is wait
                    if (ok) {
                        s->worker_meta[i].signal = BASIC_SCHED_SIGNAL_OK;
                        // basic_sched_print_stat(s, i, "ok");
                    }
                    // basic_sched_print_stat(s, i, "not ok");
                } else if (s->worker_meta[i].signal == BASIC_SCHED_SIGNAL_DONE) {
                    basic_sched_cleanup(s, i);
                    // basic_sched_print_stat(s, i, "cleanup");
                    s->worker_meta[i].request = NULL;
                    s->worker_meta[i].signal = BASIC_SCHED_SIGNAL_WAIT;
                } // else is ok which means the txn is running
            }
        }
    }
    return NULL;
}

void basic_sched_request(struct basic_sched *s, const struct basic_sched_request *const r)
{
    const u64 tid = r->tid;
    // sanity check
    if (s->worker_meta[tid].request || s->worker_meta[tid].signal != BASIC_SCHED_SIGNAL_WAIT){
        printf("basic_sched: worker %lu request is not NULL when requesting\n", tid);
        fflush(stdout);
        exit(1);
    }
    // put request in
    s->worker_meta[tid].request = r;
    // wait
    while (s->worker_meta[tid].signal != BASIC_SCHED_SIGNAL_OK);
    return;
}

void basic_sched_finish(struct basic_sched *const s, const struct basic_sched_request *const r)
{
    const u64 tid = r->tid;
    // sanity check
    if (s->worker_meta[tid].request != r|| s->worker_meta[tid].signal != BASIC_SCHED_SIGNAL_OK) {
        printf("basic_sched: worker %lu finish but not status mismatch\n", tid);
        fflush(stdout);
        exit(1);
    }
    s->worker_meta[tid].signal = BASIC_SCHED_SIGNAL_DONE;
    while (s->worker_meta[tid].signal != BASIC_SCHED_SIGNAL_WAIT);
    // the scheduler thread will clean request internally
    return;
}

#endif
