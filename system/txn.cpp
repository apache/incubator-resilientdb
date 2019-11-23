#include "helper.h"
#include "txn.h"
#include "wl.h"
#include "query.h"
#include "thread.h"
#include "mem_alloc.h"
#include "msg_queue.h"
#include "pool.h"
#include "message.h"
#include "ycsb_query.h"
#include "array.h"

void TxnStats::init()
{
    starttime = 0;
    wait_starttime = get_sys_clock();
    total_process_time = 0;
    process_time = 0;
    total_local_wait_time = 0;
    local_wait_time = 0;
    total_remote_wait_time = 0;
    remote_wait_time = 0;
    write_cnt = 0;
    abort_cnt = 0;

    total_work_queue_time = 0;
    work_queue_time = 0;
    total_work_queue_cnt = 0;
    work_queue_cnt = 0;
    total_msg_queue_time = 0;
    msg_queue_time = 0;
    total_abort_time = 0;
    time_start_pre_prepare = 0;
    time_start_prepare = 0;
    time_start_commit = 0;
    time_start_execute = 0;

    clear_short();
}

void TxnStats::clear_short()
{

    work_queue_time_short = 0;
    cc_block_time_short = 0;
    cc_time_short = 0;
    msg_queue_time_short = 0;
    process_time_short = 0;
    network_time_short = 0;
}

void TxnStats::reset()
{
    wait_starttime = get_sys_clock();
    total_process_time += process_time;
    process_time = 0;
    total_local_wait_time += local_wait_time;
    local_wait_time = 0;
    total_remote_wait_time += remote_wait_time;
    remote_wait_time = 0;
    write_cnt = 0;

    total_work_queue_time += work_queue_time;
    work_queue_time = 0;
    total_work_queue_cnt += work_queue_cnt;
    work_queue_cnt = 0;
    total_msg_queue_time += msg_queue_time;
    msg_queue_time = 0;

    clear_short();
}

void TxnStats::abort_stats(uint64_t thd_id)
{
    total_process_time += process_time;
    total_local_wait_time += local_wait_time;
    total_remote_wait_time += remote_wait_time;
    total_work_queue_time += work_queue_time;
    total_msg_queue_time += msg_queue_time;
    total_work_queue_cnt += work_queue_cnt;
    assert(total_process_time >= process_time);
}

void TxnStats::commit_stats(uint64_t thd_id, uint64_t txn_id, uint64_t batch_id, uint64_t timespan_long,
                            uint64_t timespan_short)
{
    total_process_time += process_time;
    total_local_wait_time += local_wait_time;
    total_remote_wait_time += remote_wait_time;
    total_work_queue_time += work_queue_time;
    total_msg_queue_time += msg_queue_time;
    total_work_queue_cnt += work_queue_cnt;
    assert(total_process_time >= process_time);

    if (IS_LOCAL(txn_id))
    {
        PRINT_LATENCY("lat_s %ld %ld %f %f %f %f\n", txn_id, work_queue_cnt, (double)timespan_short / BILLION, (double)work_queue_time / BILLION, (double)msg_queue_time / BILLION, (double)process_time / BILLION);
    }
    else
    {
        PRINT_LATENCY("lat_rs %ld %ld %f %f %f %f\n", txn_id, work_queue_cnt, (double)timespan_short / BILLION, (double)total_work_queue_time / BILLION, (double)total_msg_queue_time / BILLION, (double)total_process_time / BILLION);
    }

    if (!IS_LOCAL(txn_id))
    {
        return;
    }
}

void Transaction::init()
{
    txn_id = UINT64_MAX;
    batch_id = UINT64_MAX;

    reset(0);
}

void Transaction::reset(uint64_t pool_id)
{
    rc = RCOK;
}

void Transaction::release(uint64_t pool_id)
{
    DEBUG("Transaction release\n");
}

void TxnManager::init(uint64_t pool_id, Workload *h_wl)
{
    if (!txn)
    {
        DEBUG_M("Transaction alloc\n");
        txn_pool.get(pool_id, txn);
    }

    if (!query)
    {
        DEBUG_M("TxnManager::init Query alloc\n");
        qry_pool.get(pool_id, query);
    }

    sem_init(&rsp_mutex, 0, 1);
    return_id = UINT64_MAX;

    this->h_wl = h_wl;

    txn_ready = true;

    prepared = false;
    committed_local = false;
    prep_rsp_cnt = 2 * g_min_invalid_nodes;
    commit_rsp_cnt = prep_rsp_cnt + 1;
    chkpt_cnt = 2 * g_min_invalid_nodes;

    txn_stats.init();
}

// reset after abort
void TxnManager::reset()
{
    rsp_cnt = 0;
    aborted = false;
    return_id = UINT64_MAX;
    //twopl_wait_start = 0;

    assert(txn);
    assert(query);
    txn->reset(get_thd_id());

    // Stats
    txn_stats.reset();
}

void TxnManager::release(uint64_t pool_id)
{

    uint64_t tid = get_txn_id();

    qry_pool.put(pool_id, query);
    query = NULL;
    txn_pool.put(pool_id, txn);
    txn = NULL;

    txn_ready = true;

    hash.clear();
    prepared = false;

    prep_rsp_cnt = 2 * g_min_invalid_nodes;
    commit_rsp_cnt = prep_rsp_cnt + 1;
    chkpt_cnt = 2 * g_min_invalid_nodes + 1;
    release_all_messages(tid);

    txn_stats.init();
}

void TxnManager::reset_query()
{
    ((YCSBQuery *)query)->reset();
}

RC TxnManager::commit()
{
    DEBUG("Commit %ld\n", get_txn_id());

    commit_stats();
    return Commit;
}

RC TxnManager::start_commit()
{
    RC rc = RCOK;
    DEBUG("%ld start_commit RO?\n", get_txn_id());
    return rc;
}

int TxnManager::received_response(RC rc)
{
    assert(txn->rc == RCOK);
    if (txn->rc == RCOK)
        txn->rc = rc;

    --rsp_cnt;

    return rsp_cnt;
}

bool TxnManager::waiting_for_response()
{
    return rsp_cnt > 0;
}

void TxnManager::commit_stats()
{
    uint64_t commit_time = get_sys_clock();
    uint64_t timespan_short = commit_time - txn_stats.restart_starttime;
    uint64_t timespan_long = commit_time - txn_stats.starttime;
    INC_STATS(get_thd_id(), total_txn_commit_cnt, 1);

    if (!IS_LOCAL(get_txn_id()))
    {
        txn_stats.commit_stats(get_thd_id(), get_txn_id(), get_batch_id(), timespan_long, timespan_short);
        return;
    }

    INC_STATS(get_thd_id(), txn_cnt, 1);
    INC_STATS(get_thd_id(), txn_run_time, timespan_long);
    INC_STATS(get_thd_id(), single_part_txn_cnt, 1);

    txn_stats.commit_stats(get_thd_id(), get_txn_id(), get_batch_id(), timespan_long, timespan_short);
}

void TxnManager::register_thread(Thread *h_thd)
{
    this->h_thd = h_thd;
}

void TxnManager::set_txn_id(txnid_t txn_id)
{
    txn->txn_id = txn_id;
}

txnid_t TxnManager::get_txn_id()
{
    return txn->txn_id;
}

Workload *TxnManager::get_wl()
{
    return h_wl;
}

uint64_t TxnManager::get_thd_id()
{
    if (h_thd)
        return h_thd->get_thd_id();
    else
        return 0;
}

BaseQuery *TxnManager::get_query()
{
    return query;
}

void TxnManager::set_query(BaseQuery *qry)
{
    query = qry;
}

uint64_t TxnManager::incr_rsp(int i)
{
    uint64_t result;
    sem_wait(&rsp_mutex);
    result = ++this->rsp_cnt;
    sem_post(&rsp_mutex);
    return result;
}

uint64_t TxnManager::decr_rsp(int i)
{
    uint64_t result;
    sem_wait(&rsp_mutex);
    result = --this->rsp_cnt;
    sem_post(&rsp_mutex);
    return result;
}

RC TxnManager::validate()
{
    return RCOK;
}

/* Generic Helper functions. */

string TxnManager::get_hash()
{
    return hash;
}

void TxnManager::set_hash(string hsh)
{
    hash = hsh;
    hashSize = hash.length();
}

uint64_t TxnManager::get_hashSize()
{
    return hashSize;
}

bool TxnManager::is_chkpt_ready()
{
    return chkpt_flag;
}

void TxnManager::set_chkpt_ready()
{
    chkpt_flag = true;
}

uint64_t TxnManager::decr_chkpt_cnt()
{
    chkpt_cnt--;
    return chkpt_cnt;
}

uint64_t TxnManager::get_chkpt_cnt()
{
    return chkpt_cnt;
}

/* Helper functions for PBFT. */
void TxnManager::set_prepared()
{
    prepared = true;
}

bool TxnManager::is_prepared()
{
    return prepared;
}

uint64_t TxnManager::decr_prep_rsp_cnt()
{
    prep_rsp_cnt--;
    return prep_rsp_cnt;
}

uint64_t TxnManager::get_prep_rsp_cnt()
{
    return prep_rsp_cnt;
}

/************************************/

/* Helper functions for PBFT. */

void TxnManager::set_committed()
{
    committed_local = true;
}

bool TxnManager::is_committed()
{
    return committed_local;
}

uint64_t TxnManager::decr_commit_rsp_cnt()
{
    commit_rsp_cnt--;
    return commit_rsp_cnt;
}

uint64_t TxnManager::get_commit_rsp_cnt()
{
    return commit_rsp_cnt;
}

/*****************************/

//broadcasts prepare message to all nodes
void TxnManager::send_pbft_prep_msgs()
{
    //printf("%ld Send PBFT_PREP_MSG message to %d nodes\n", get_txn_id(), g_node_cnt - 1);
    //fflush(stdout);

    Message *msg = Message::create_message(this, PBFT_PREP_MSG);
    PBFTPrepMessage *pmsg = (PBFTPrepMessage *)msg;

#if LOCAL_FAULT == true || VIEW_CHANGES
    if (get_prep_rsp_cnt() > 0)
    {
        decr_prep_rsp_cnt();
    }
#endif

    vector<string> emptyvec;
    vector<uint64_t> dest;
    for (uint64_t i = 0; i < g_node_cnt; i++)
    {
        if (i == g_node_id)
        {
            continue;
        }
        dest.push_back(i);
    }

    msg_queue.enqueue(get_thd_id(), pmsg, emptyvec, dest);
    dest.clear();
}

//broadcasts commit message to all nodes
void TxnManager::send_pbft_commit_msgs()
{
    //cout << "Send PBFT_COMMIT_MSG messages " << get_txn_id() << "\n";
    //fflush(stdout);

    Message *msg = Message::create_message(this, PBFT_COMMIT_MSG);
    PBFTCommitMessage *cmsg = (PBFTCommitMessage *)msg;

#if LOCAL_FAULT == true || VIEW_CHANGES
    if (get_commit_rsp_cnt() > 0)
    {
        decr_commit_rsp_cnt();
    }
#endif

    vector<string> emptyvec;
    vector<uint64_t> dest;
    for (uint64_t i = 0; i < g_node_cnt; i++)
    {
        if (i == g_node_id)
        {
            continue;
        }
        dest.push_back(i);
    }

    msg_queue.enqueue(get_thd_id(), cmsg, emptyvec, dest);
    dest.clear();
}

#if !TESTING_ON

void TxnManager::release_all_messages(uint64_t txn_id)
{
    if ((txn_id + 3) % get_batch_size() == 0)
    {
        allsign.clear();
    }
    else if ((txn_id + 1) % get_batch_size() == 0)
    {
        info_prepare.clear();
        info_commit.clear();
    }
}

#endif // !TESTING

//broadcasts checkpoint message to all nodes
void TxnManager::send_checkpoint_msgs()
{
    DEBUG("%ld Send PBFT_CHKPT_MSG message to %d\n nodes", get_txn_id(), g_node_cnt - 1);

    Message *msg = Message::create_message(this, PBFT_CHKPT_MSG);
    CheckpointMessage *ckmsg = (CheckpointMessage *)msg;

    vector<string> emptyvec;
    vector<uint64_t> dest;
    for (uint64_t i = 0; i < g_node_cnt; i++)
    {
        if (i == g_node_id)
        {
            continue;
        }
        dest.push_back(i);
    }

    msg_queue.enqueue(get_thd_id(), ckmsg, emptyvec, dest);
    dest.clear();
}
