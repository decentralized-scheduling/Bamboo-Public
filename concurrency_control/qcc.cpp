#include "txn.h"
#include "row.h"
#include "row_qcc.h"

#if CC_ALG == QCC

RC
txn_man::qcc_commit()
{
    for (int i = 0; i < row_cnt; i++) {
        Access * access = accesses[i];
        if (access->type != WR) {
            // make sure here the data is exactly the original row w/o modifications
            assert(access->data == access->orig_row);
            continue;
        }
        // then here.. make sure we copy from buf to orig row
        assert(access->data == access->buf);

        access->orig_row->manager->write(access->data);
    }
    return RCOK;
}
#endif
