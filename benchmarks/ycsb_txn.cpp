#include "global.h"
#include "helper.h"
#include "ycsb.h"
#include "ycsb_query.h"
#include "wl.h"
#include "thread.h"
#include "mem_alloc.h"
#include "query.h"
#include "msg_queue.h"

void YCSBTxnManager::init(uint64_t thd_id, Workload *h_wl)
{
    TxnManager::init(thd_id, h_wl);
    _wl = (YCSBWorkload *)h_wl;
    reset();
}

void YCSBTxnManager::reset()
{
    TxnManager::reset();
}

RC YCSBTxnManager::run_txn()
{
    uint64_t starttime = get_sys_clock();

    YCSBQuery *ycsb_query = (YCSBQuery *)query;
    ycsb_request *yreq;
    for (uint i = 0; i < ycsb_query->requests.size(); i++)
    {
        yreq = ycsb_query->requests[i];
        db->Put(std::to_string(yreq->key), std::to_string(yreq->value));
    }

    uint64_t curr_time = get_sys_clock();
    txn_stats.process_time += curr_time - starttime;
    txn_stats.process_time_short += curr_time - starttime;
    txn_stats.wait_starttime = get_sys_clock();

    return RCOK;
}
