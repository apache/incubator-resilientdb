#ifndef _SC_TXN_H_
#define _SC_TXN_H_
#include "global.h"
#include "txn.h"
#include "wl.h"

#if BANKING_SMART_CONTRACT

class SCWorkload : public Workload
{
public:
    RC init();
    RC get_txn_man(TxnManager *&txn_manager);
    int key_to_part(uint64_t key);
};

class SmartContractTxn : public TxnManager
{
public:
    void init(uint64_t thd_id, Workload *h_wl);
    void reset();
    RC run_txn();

private:
   SCWorkload *_wl;
};

#endif
#endif
