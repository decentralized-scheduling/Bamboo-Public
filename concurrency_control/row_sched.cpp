#include "txn.h"
#include "row.h"
#include "row_sched.h"
#include "mem_alloc.h"

void
Row_sched::init(row_t * row) {
	_row = row;
}
