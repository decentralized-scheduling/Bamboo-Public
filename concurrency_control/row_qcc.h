#ifndef _ROW_QCC_H
#define _ROW_QCC_H

class table_t;
class txn_man;

class Row_qcc {
public:
	void 				init(row_t * row);
	void				write(row_t * data);
private:
	row_t * 			_row;
};

#endif
