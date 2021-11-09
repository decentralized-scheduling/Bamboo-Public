#ifndef _ROW_SCHED_H
#define _ROW_SCHED_H

#include "row.h"

class Row_sched {
public:
	void 				init(row_t * row);
private:
	row_t * 			_row;
};

#endif
