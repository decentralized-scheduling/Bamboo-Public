#ifndef _ROW_OL_H
#define _ROW_OL_H

class table_t;
class txn_man;

class Row_ol {
public:
	void 				init(row_t * row);
    // simple locks, and always non-blocking
    void                lock(access_t acc);
    void                unlock(access_t acc);
    uint64_t            rank;
private:
	row_t * 			_row;
    pthread_rwlock_t    _lock;
};

#endif
