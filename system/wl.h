#ifndef _WORKLOAD_H_
#define _WORKLOAD_H_

#include "global.h"

class TxnManager;
class Thread;

class Workload
{
public:
    // initialize
    virtual RC init();
    virtual RC get_txn_man(TxnManager *&txn_manager) = 0;
    uint64_t done_cnt;
    uint64_t txn_cnt;
};

#endif
