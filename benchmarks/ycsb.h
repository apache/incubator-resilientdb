#ifndef _SYNTH_BM_H_
#define _SYNTH_BM_H_

#include "wl.h"
#include "txn.h"
#include "global.h"
#include "helper.h"

class YCSBQuery;

class YCSBQueryMessage;

class ycsb_request;

class YCSBWorkload : public Workload
{
public:
    RC init();
    RC get_txn_man(TxnManager *&txn_manager);
    int key_to_part(uint64_t key);

private:
    pthread_mutex_t insert_lock;
    //  For parallel initialization
    static int next_tid;
};

class YCSBTxnManager : public TxnManager
{
public:
    void init(uint64_t thd_id, Workload *h_wl);
    void reset();
    RC run_txn();

private:
    YCSBWorkload *_wl;
};

#endif
