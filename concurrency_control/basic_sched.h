#ifndef _BASIC_SCHED_H
#define _BASIC_SCHED_H

#include "row_sched.h"

#define BASIC_SCHED_SIZE (1lu << 14)

// per-txn signal
enum basic_sched_signal {
    BASIC_SCHED_SIGNAL_WAIT = 0,
    BASIC_SCHED_SIGNAL_OK = 1,
    BASIC_SCHED_SIGNAL_DONE = 2,
};

// per-object status
struct basic_sched_status {
    u32 nr_readers;
    u32 nr_writers;
    u8 write_requested; // avoid starvation
};

struct basic_sched_request {
    u64 tid;
    u64 nr_requests;
    struct {
        u64 id;
        access_t type;
    } requests[MAX_ROW_PER_TXN];
};

// remember to cl alloc this struct
struct basic_sched {
    union {
        volatile u8 valid;
        u64 _0[8];
    };
    // per-worker memory, separated by cache lines
    union {
        struct {
            const struct basic_sched_request *request;
            volatile enum basic_sched_signal signal;
        };
        u64 _[8];
    } worker_meta[THREAD_CNT];
    // private for scheduler
    u64 _1[8];
    struct basic_sched_status status[BASIC_SCHED_SIZE];
};

static_assert(sizeof(((struct basic_sched *)NULL)->worker_meta[0]) == 64);

// for scheduler
struct basic_sched *basic_sched_create();

void *basic_sched_run(void *sptr);

// for clients
void basic_sched_request(struct basic_sched* const s, const struct basic_sched_request *const r);

void basic_sched_finish(struct basic_sched *const s, const struct basic_sched_request *const r);

#endif
