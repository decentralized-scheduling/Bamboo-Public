#include "txn.h"
#include "row.h"
#include "row_qcc.h"
#include "mem_alloc.h"

#if CC_ALG == QCC

void
Row_qcc::init(row_t * row) {
	_row = row;
}

void
Row_qcc::write(row_t * data) {
	_row->copy(data);
}

#endif
