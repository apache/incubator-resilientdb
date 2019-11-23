#include "work_queue.h"
#include "mem_alloc.h"
#include "query.h"
#include "message.h"
#include "client_query.h"
#include <boost/lockfree/queue.hpp>

void QWorkQueue::init()
{

    last_sched_dq = NULL;
    sched_ptr = 0;
    seq_queue = new boost::lockfree::queue<work_queue_entry *>(0);

    // Queue for worker thread 0.
    uint64_t effective_queue_cnt = 1;

#if EXECUTION_THREAD
    effective_queue_cnt += indexSize;
#endif

    // A queue for checkpoint messages.
    effective_queue_cnt++;

    cout << "Total queues: " << effective_queue_cnt << "\n";
    fflush(stdout);

    work_queue = new boost::lockfree::queue<work_queue_entry *> *[effective_queue_cnt];
    for (uint64_t i = 0; i < effective_queue_cnt; i++)
    {
        work_queue[i] = new boost::lockfree::queue<work_queue_entry *>(0);
    }

    new_txn_queue = new boost::lockfree::queue<work_queue_entry *>(0);
    sched_queue = new boost::lockfree::queue<work_queue_entry *> *[g_node_cnt];
    for (uint64_t i = 0; i < g_node_cnt; i++)
    {
        sched_queue[i] = new boost::lockfree::queue<work_queue_entry *>(0);
    }
}

#if ENABLE_PIPELINE
// If pipeline is enabled, then different worker_threads have different queues.
void QWorkQueue::enqueue(uint64_t thd_id, Message *msg, bool busy)
{
    uint64_t starttime = get_sys_clock();
    assert(msg);
    DEBUG_M("QWorkQueue::enqueue work_queue_entry alloc\n");
    work_queue_entry *entry = (work_queue_entry *)mem_allocator.align_alloc(sizeof(work_queue_entry));
    entry->msg = msg;
    entry->rtype = msg->rtype;
    entry->txn_id = msg->txn_id;
    entry->batch_id = msg->batch_id;
    entry->starttime = get_sys_clock();
    assert(ISSERVER || ISREPLICA);
    DEBUG("Work Enqueue (%ld,%ld) %d\n", entry->txn_id, entry->batch_id, entry->rtype);

    if (msg->rtype == CL_QRY || msg->rtype == CL_BATCH)
    {
        if (g_node_id == get_current_view(thd_id))
        {
            //cout << "Placing \n";
            while (!new_txn_queue->push(entry) && !simulation->is_done())
            {
            }
        }
        else
        {
            assert(entry->rtype < 100);
            while (!work_queue[0]->push(entry) && !simulation->is_done())
            {
            }
        }
    }
    else if (msg->rtype == BATCH_REQ)
    {
        // Queue for Thread for ordered sending of batches.
        assert(entry->rtype < 100);
        if (g_node_id != get_current_view(thd_id))
        {
            while (!work_queue[0]->push(entry) && !simulation->is_done())
            {
            }
        }
    }
    else if (msg->rtype == EXECUTE_MSG)
    {
        uint64_t bid = ((msg->txn_id + 2) - get_batch_size()) / get_batch_size();
        uint64_t qid = bid % indexSize;
        while (!work_queue[qid + 1]->push(entry) && !simulation->is_done())
        {
        }
    }
    else if (msg->rtype == PBFT_CHKPT_MSG)
    {
        while (!work_queue[indexSize + 1]->push(entry) && !simulation->is_done())
        {
        }
    }
    else
    {
        assert(entry->rtype < 100);
        while (!work_queue[0]->push(entry) && !simulation->is_done())
        {
        }
    }

    INC_STATS(thd_id, work_queue_enqueue_time, get_sys_clock() - starttime);
    INC_STATS(thd_id, work_queue_enq_cnt, 1);
}

Message *QWorkQueue::dequeue(uint64_t thd_id)
{
    uint64_t starttime = get_sys_clock();
    assert(ISSERVER || ISREPLICA);
    Message *msg = NULL;
    work_queue_entry *entry = NULL;

    bool valid = false;

    // Thread 0 only looks at work queue
    if (thd_id == 0)
    {
        valid = work_queue[0]->pop(entry);
    }

    UInt32 tcount = g_thread_cnt - g_execute_thd - g_btorder_thd;

    if (thd_id >= tcount && thd_id < (tcount + g_execute_thd))
    {
        // Thread for handling execute messages.
        uint64_t bid = ((get_expectedExecuteCount() + 2) - get_batch_size()) / get_batch_size();
        uint64_t qid = bid % indexSize;
        valid = work_queue[qid + 1]->pop(entry);
    }
    else if (thd_id >= tcount + g_execute_thd)
    {
        // Thread for handling checkpoint messages.
        valid = work_queue[indexSize + 1]->pop(entry);
    }

    if (!valid)
    {
        // Allowing new transactions to be accessed by batching threads.
        if (thd_id > 0 && thd_id <= tcount - 1)
        {
            valid = new_txn_queue->pop(entry);
        }
    }

    if (valid)
    {
        msg = entry->msg;
        assert(msg);
        uint64_t queue_time = get_sys_clock() - entry->starttime;
        INC_STATS(thd_id, work_queue_wait_time, queue_time);
        INC_STATS(thd_id, work_queue_cnt, 1);

        msg->wq_time = queue_time;
        DEBUG("Work Dequeue (%ld,%ld)\n", entry->txn_id, entry->batch_id);
        mem_allocator.free(entry, sizeof(work_queue_entry));
        INC_STATS(thd_id, work_queue_dequeue_time, get_sys_clock() - starttime);
    }

    return msg;
}

#endif // ENABLE_PIPELINE == true
