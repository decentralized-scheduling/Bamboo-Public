#include "global.h"
#include "helper.h"
#include "ycsb.h"
#include "ycsb_query.h"
#include "wl.h"
#include "thread.h"
#include "table.h"
#include "row.h"
#include "index_hash.h"
#include "index_btree.h"
#include "catalog.h"
#include "manager.h"
#include "row_lock.h"
#include "row_ts.h"
#include "row_mvcc.h"
#include "row_ol.h"
#include "mem_alloc.h"
#include "query.h"

void ycsb_txn_man::init(thread_t * h_thd, workload * h_wl, uint64_t thd_id) {
    txn_man::init(h_thd, h_wl, thd_id);
    _wl = (ycsb_wl *) h_wl;
}

#if CC_ALG == ORDERED_LOCK
// the semantics of this function is to locking all items in order according to their access types
void ycsb_txn_man::ol_prepare_ycsb(const ycsb_query *const query)
{
    nr_rows = query->request_cnt;
    for (u64 i=0; i < nr_rows; i++) {
        ycsb_request *req = &query->requests[i];
        rows[i].type = req->rtype;
        const u64 key = req->key;
        int part_id = _wl->key_to_part(key);
        rows[i].row_item = index_read(_wl->the_index, key, part_id);
    }

    // now we have read the index and got all the keys, lets sort them
    ol_sort_rows();

    // sorted, lock them then
    for (u64 i=0; i<nr_rows; i++) {
        // lock, or wait
        ((row_t *)(rows[i].row_item->location))->manager->lock(rows[i].type);
    }
}

void ycsb_txn_man::ol_finish_ycsb()
{
    // unlock them
    for (u64 i=nr_rows; i>0; i--) {
        // lock, or wait
        ((row_t *)(rows[i-1].row_item->location))->manager->unlock(rows[i-1].type);
    }
    nr_rows = 0;
}
#endif

#if CC_ALG == BASIC_SCHED
void ycsb_txn_man::bs_prepare_ycsb(const ycsb_query *const query)
{
    request.tid = this->get_thd_id();
    u64 nr_rows = query->request_cnt;
    for (u64 i=0; i<nr_rows; i++) {
        ycsb_request *req = &query->requests[i];
        const u64 key = req->key;
        request.requests[i].id = key;
        request.requests[i].type = req->rtype;
    }
    request.nr_requests = nr_rows;

    bs_regulate_request();

    struct basic_sched *const s = this->get_wl()->s;
    basic_sched_request(s, &request);
    return;
}

void ycsb_txn_man::bs_finish_ycsb()
{
    struct basic_sched *const s = this->get_wl()->s;
    basic_sched_finish(s, &request);
    return;
}
#endif

#if CC_ALG == QCC
struct qcc_txn *ycsb_txn_man::qcc_prepare_ycsb(const ycsb_query *const query)
{
    struct qcc *const q = this->get_wl()->q;
    struct qcc_txn *const txn = qcc_txn_acquire(q, this->get_thd_id());
    assert(txn);

    nr_rows = query->request_cnt;

    for (u64 i=0; i < nr_rows; i++) {
        ycsb_request *req = &query->requests[i];
        const u64 key = req->key;
        rows[i].rid = key;
        if (req->rtype == RD) {
            rows[i].type = QCC_TYPE_RD;
        } else {
            rows[i].type = QCC_TYPE_WR;
        }
        int part_id = _wl->key_to_part(key);
        row_buffer[i] = index_read(_wl->the_index, key, part_id);
        assert(row_buffer[i]);
    }
    qcc_txn_enqueue(q, txn, &rows[0], nr_rows, &queues[0], &nr_queues);
    return txn;
    // always return..
}
#endif

RC ycsb_txn_man::run_txn(base_query * query) {
#if CC_ALG == QCC
    struct qcc *const q = this->get_wl()->q;
    RC rc;
    u8 local_clean = 0;
    u8 try_cnt = 0;
    const u64 max_try_cnt = q->nr_workers / 2 + 1;

    struct qcc_txn *const txn = qcc_prepare_ycsb((ycsb_query *)query);

    if (!txn) {
        // finish() called here!
        rc = finish(Abort);
        assert(rc == Abort);
    } else {
        while (!qcc_txn_try_wait(q, txn) && try_cnt < max_try_cnt) {
            if (local_clean && qcc_txn_snapshot_consistent(q, txn, &rows[0], nr_rows, nr_rows)) {
                continue;
            }
            try_cnt++;
            if (try_cnt) {
                cleanup(Abort); // remove the results in last attempt
            }
            local_clean = 0; // mark local status as dirty
            qcc_txn_snapshot(q, txn, &rows[0], nr_rows);
            rc = run_txn_r(query); // try run once locally
            if (rc == RCOK) {
                local_clean = 1;
            }
        }
        qcc_txn_wait(q, txn); // wait more
        if (!local_clean || !qcc_txn_snapshot_consistent(q, txn, &rows[0], nr_rows, nr_rows)) {
            // really execute unless:
            // 1. clean mark is set and
            // 2. the snapshot is consistent wrd last try
            if (try_cnt) {
                cleanup(Abort);
            }
            rc = run_txn_r(query); // try run once locally
        } else {
            assert(rc == RCOK);
        }
        rc = finish(rc);
    }
    qcc_txn_finish(q, txn, queues, nr_queues); // announce!
    return rc;
#elif CC_ALG == ORDERED_LOCK
    ol_prepare_ycsb((ycsb_query *)query);
    RC rc = run_txn_r(query);
    ol_finish_ycsb();
    rc = finish(rc);
    return rc;
#elif CC_ALG == BASIC_SCHED
    bs_prepare_ycsb((ycsb_query *)query);
    RC rc = run_txn_r(query);
    bs_finish_ycsb();
    rc = finish(rc);
    return rc;
#else
    return run_txn_r(query);
#endif
}

// for QCC, this func should not call finish()
RC ycsb_txn_man::run_txn_r(base_query * query) {
    RC rc;
    ycsb_query * m_query = (ycsb_query *) query;
    ycsb_wl * wl = (ycsb_wl *) h_wl;
    itemid_t * m_item = NULL;
#if CC_ALG == BAMBOO && (THREAD_CNT != 1)
    int access_id;
    retire_threshold = (uint32_t) floor(m_query->request_cnt * (1 - g_last_retire));
#else
    row_cnt = 0;
#endif

    // if long txn and not rerun aborted txn, generate queries
    if (unlikely(m_query->is_long && !(m_query->rerun))) {
        uint64_t starttime = get_sys_clock();
        m_query->gen_requests(h_thd->get_thd_id(), h_wl);
        DEC_STATS(h_thd->get_thd_id(), run_time, get_sys_clock() - starttime);
    }

    for (uint32_t rid = 0; rid < m_query->request_cnt; rid ++) {
        ycsb_request * req = &m_query->requests[rid];
        int part_id = wl->key_to_part( req->key );
        bool finish_req = false;
        UInt32 iteration = 0;
        while ( !finish_req ) {
            if (iteration == 0) {
#if CC_ALG == QCC
                m_item = row_buffer[rid];
#else
                m_item = index_read(_wl->the_index, req->key, part_id);
#endif
            }
#if INDEX_STRUCT == IDX_BTREE
            else {
                _wl->the_index->index_next(get_thd_id(), m_item);
                if (m_item == NULL)
                    break;
            }
#endif
            row_t * row = ((row_t *)m_item->location);
            row_t * row_local;
            access_t type = req->rtype;
            //printf("[txn-%lu] start %d requests at key %lu\n", get_txn_id(), rid, req->key);
            row_local = get_row(row, type);
            if (row_local == NULL) {
                rc = Abort;
                goto final;
            }
#if CC_ALG == BAMBOO && (THREAD_CNT != 1)
            access_id = row_cnt - 1;
#endif

            // Computation //
            // Only do computation when there are more than 1 requests.
            if (m_query->request_cnt > 1) {
                if (req->rtype == RD || req->rtype == SCAN) {
//                  for (int fid = 0; fid < schema->get_field_cnt(); fid++) {
                        int fid = 0;
                        char * data = row_local->get_data();
                        __attribute__((unused)) uint64_t fval = *(uint64_t *)(&data[fid * 10]);
//                  }
                } else {
                    assert(req->rtype == WR);
//					for (int fid = 0; fid < schema->get_field_cnt(); fid++) {
                        int fid = 0;
#if (CC_ALG == BAMBOO) || (CC_ALG == WOUND_WAIT)
                        char * data = row_local->get_data();
#else
                        char * data = row->get_data();
#endif
                        *(uint64_t *)(&data[fid * 10]) = 0;
//					}
                }
            }


            iteration ++;
            if (req->rtype == RD || req->rtype == WR || iteration == req->scan_len)
                finish_req = true;
#if (CC_ALG == BAMBOO) && (THREAD_CNT != 1)
            // retire write txn
            if (finish_req && (req->rtype == WR) && (rid <= retire_threshold)) {
            	//printf("[txn-%lu] retire %d requests\n", get_txn_id(), rid);
                if (retire_row(access_id) == Abort)
                  return finish(Abort); // this is bamboo..
            }
#endif
        }
    }
    rc = RCOK;
final:
#if CC_ALG != QCC && CC_ALG != ORDERED_LOCK && CC_ALG != BASIC_SCHED
    rc = finish(rc); // only call finish unless it is QCC or ORDERED_LOCK or BASIC_SCHED
                     // because both those CCs have decoupled finish() phase
#endif
    return rc;
}

