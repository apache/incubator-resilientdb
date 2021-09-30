#include "work_queue.h"
#include "mem_alloc.h"
#include "query.h"
#include "message.h"
#include "client_query.h"
#include <boost/lockfree/queue.hpp>

#if !ENABLE_PIPELINE

// If pipeline is disabled then only worker thread 0 and execution thread
// are working at primary replica.
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
            while (!new_txn_queue->push(entry) && !simulation->is_done())
            {
            }
        }
        else
        {
            while (!work_queue[0]->push(entry) && !simulation->is_done())
            {
            }
        }
    }
    else
    {
        assert(entry->rtype < 100);
        if (msg->rtype == EXECUTE_MSG || msg->rtype == PBFT_CHKPT_MSG)
        {
            while (!work_queue[indexSize + 1]->push(entry) && !simulation->is_done())
            {
            }
        }
        else
        {
            while (!work_queue[0]->push(entry) && !simulation->is_done())
            {
            }
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

#if EXECUTION_THREAD
    UInt32 tcount = g_thread_cnt - g_checkpointing_thread_cnt - g_execution_thread_cnt;
    if (thd_id >= tcount + g_checkpointing_thread_cnt)
    {
        valid = work_queue[indexSize + 1]->pop(entry);
    }
#endif

    if (!valid)
    {
        // Allowing new transactions to be accessed by batching threads.
        if (thd_id == 0)
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

#endif // ENABLE_PIPELINE == false
