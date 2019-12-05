#ifndef _TXN_H_
#define _TXN_H_

#include "global.h"
#include "helper.h"
#include "semaphore.h"
#include "array.h"
#include "message.h"

class Workload;
class Thread;
class table_t;
class BaseQuery;
class TxnQEntry;
class YCSBQuery;

class Transaction
{
public:
    void init();
    void reset(uint64_t pool_id);
    void release(uint64_t pool_id);
    txnid_t txn_id;
    uint64_t batch_id;
    RC rc;
};

class TxnStats
{
public:
    void init();
    void clear_short();
    void reset();
    void abort_stats(uint64_t thd_id);
    void commit_stats(uint64_t thd_id, uint64_t txn_id, uint64_t batch_id, uint64_t timespan_long, uint64_t timespan_short);
    uint64_t starttime;
    uint64_t restart_starttime;
    uint64_t wait_starttime;
    uint64_t write_cnt;
    uint64_t abort_cnt;
    double total_process_time;
    double process_time;
    double total_local_wait_time;
    double local_wait_time;
    double total_remote_wait_time; // time waiting for a remote response
    double remote_wait_time;
    double total_abort_time;     // time spent in aborted query land
    double total_msg_queue_time; // time spent on outgoing queue
    double msg_queue_time;
    double total_work_queue_time; // time spent on work queue
    double work_queue_time;
    uint64_t total_work_queue_cnt;
    uint64_t work_queue_cnt;

    // short stats
    double work_queue_time_short;
    double cc_block_time_short;
    double cc_time_short;
    double msg_queue_time_short;
    double process_time_short;
    double network_time_short;

    double lat_network_time_start;
    double lat_other_time_start;

    //PBFT Stats
    double time_start_pre_prepare;
    double time_start_prepare;
    double time_start_commit;
    double time_start_execute;
};

/*
   Execution of transactions
   Manipulates/manages Transaction (contains txn-specific data)
   Maintains BaseQuery (contains input args, info about query)
*/
class TxnManager
{
public:
    virtual ~TxnManager() {}
    virtual void init(uint64_t thd_id, Workload *h_wl);
    virtual void reset();
    void clear();
    void reset_query();
    void release(uint64_t pool_id);
    void release_all_messages(uint64_t txn_id);

    Thread *h_thd;
    Workload *h_wl;

    virtual RC run_txn() = 0;
    void register_thread(Thread *h_thd);
    uint64_t get_thd_id();
    Workload *get_wl();
    void set_txn_id(txnid_t txn_id);
    txnid_t get_txn_id();
    void set_query(BaseQuery *qry);
    BaseQuery *get_query();
    bool is_done();
    void commit_stats();

    uint64_t get_rsp_cnt() { return rsp_cnt; }
    uint64_t incr_rsp(int i);
    uint64_t decr_rsp(int i);

    RC commit();
    RC start_commit();

    bool aborted;
    uint64_t return_id;
    RC validate();

    uint64_t get_batch_id() { return txn->batch_id; }
    void set_batch_id(uint64_t batch_id) { txn->batch_id = batch_id; }

    Transaction *txn;

    BaseQuery *query;        // Client query.
    uint64_t client_startts; // Client timestamp for this transaction.
    uint64_t client_id;      // Id of client that sent this transaction.

    string hash;       // Hash of the client query.
    uint64_t hashSize; // Size of hash.
    string get_hash();
    void set_hash(string hsh);
    uint64_t get_hashSize();

    // We need to maintain one copy of the whole BatchRequests messages sent 
    // by the primary. We only maintain in last request of the batch. 
    BatchRequests *batchreq;  
    void set_primarybatch(BatchRequests *breq);

    vector<string> allsign;

    uint64_t get_abort_cnt() { return abort_cnt; }
    uint64_t abort_cnt;
    int received_response(RC rc);
    bool waiting_for_response();
    RC get_rc() { return txn->rc; }
    void set_rc(RC rc) { txn->rc = rc; }

    bool prepared = false;
    uint64_t cbatch;

    uint64_t prep_rsp_cnt;
    vector<uint64_t> info_prepare;

    uint64_t decr_prep_rsp_cnt();
    uint64_t get_prep_rsp_cnt();
    bool is_prepared();
    void set_prepared();

    void send_pbft_prep_msgs();

    uint64_t commit_rsp_cnt;
    bool committed_local = false;
    vector<uint64_t> info_commit;

    // We need to store all the complete Commit mssg in the last txn of batch.
    vector<PBFTCommitMessage *> commit_msgs; 
    void add_commit_msg(PBFTCommitMessage *pcmsg);

    uint64_t decr_commit_rsp_cnt();
    uint64_t get_commit_rsp_cnt();
    bool is_committed();
    void set_committed();

    void send_pbft_commit_msgs();

    int chkpt_cnt;
    bool chkpt_flag = false;

    bool is_chkpt_ready();
    void set_chkpt_ready();
    uint64_t decr_chkpt_cnt();
    uint64_t get_chkpt_cnt();
    void send_checkpoint_msgs();

    TxnStats txn_stats;

    bool set_ready() { return ATOM_CAS(txn_ready, 0, 1); }
    bool unset_ready() { return ATOM_CAS(txn_ready, 1, 0); }
    bool is_ready() { return txn_ready == true; }
    volatile int txn_ready;

protected:
    int rsp_cnt;
    sem_t rsp_mutex;
};

#endif
