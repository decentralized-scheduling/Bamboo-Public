#include "txn.h"
#include "row.h"
#include "row_ol.h"
#include "mem_alloc.h"

void
Row_ol::init(row_t * row) {
	_row = row;
    pthread_rwlock_init(&_lock, NULL);
    // rank, just use pointer addr for now..
    rank = (uint64_t)&_lock;
}

void
Row_ol::lock(access_t acc) {
    if (acc == WR) {
        pthread_rwlock_wrlock(&_lock);
    } else if (acc == RD) {
        pthread_rwlock_rdlock(&_lock);
    } else {
        assert(0);
    }
    return;
}

void
Row_ol::unlock(access_t acc) {
    pthread_rwlock_unlock(&_lock);
    (void)acc;
}
