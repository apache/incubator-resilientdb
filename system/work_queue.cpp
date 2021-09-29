#include "work_queue.h"
#include "mem_alloc.h"
#include "query.h"
#include "message.h"
#include "client_query.h"
#include <boost/lockfree/queue.hpp>

void QWorkQueue::push_to_queue(work_queue_entry *entry, boost::lockfree::queue<work_queue_entry *> *queue)
{
    assert(entry->rtype < 100);
    while (!queue->push(entry) && !simulation->is_done())
    {
    }
}
void QWorkQueue::init()
{
    work_queue = new boost::lockfree::queue<work_queue_entry *>(0);
    execution_queues = new boost::lockfree::queue<work_queue_entry *> *[indexSize];
    for (uint64_t i = 0; i < indexSize; i++)
    {
        execution_queues[i] = new boost::lockfree::queue<work_queue_entry *>(0);
    }
    new_txn_queue = new boost::lockfree::queue<work_queue_entry *>(0);
    checkpoint_queue = new boost::lockfree::queue<work_queue_entry *>(0);
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
        // Client Requests to batching queues
        push_to_queue(entry, new_txn_queue);
    }
    else if (msg->rtype == BATCH_REQ)
    {
        // The Primary should not receive pre-prepare message
        if (g_node_id == view_to_primary(get_current_view(thd_id)))
        {
            assert(0);
        }
        else
        {
            push_to_queue(entry, work_queue);
        }
    }
    else if (msg->rtype == EXECUTE_MSG)
    {
        // Execution Map using [indexSize] queues
        uint64_t queue_id = (msg->txn_id / get_batch_size()) % indexSize;
        push_to_queue(entry, execution_queues[queue_id]);
    }
    else if (msg->rtype == PBFT_CHKPT_MSG)
    {
        push_to_queue(entry, checkpoint_queue);
    }
    else
    {
        push_to_queue(entry, work_queue);
    }

    INC_STATS(thd_id, work_queue_enqueue_time, get_sys_clock() - starttime);
    INC_STATS(thd_id, work_queue_enq_cnt, 1);
}

Message *QWorkQueue::dequeue(uint64_t thd_id)
{
    uint64_t starttime = get_sys_clock();

    assert(ISSERVER || ISREPLICA);
    bool valid = false;
    Message *message = NULL;
    work_queue_entry *entry = NULL;

    assert(g_thread_cnt == g_worker_thread_cnt + g_batching_thread_cnt + g_checkpointing_thread_cnt + g_execution_thread_cnt);

    // Worker Threads
    if (thd_id < g_worker_thread_cnt)
    {
        valid = work_queue->pop(entry);
    }
    // Batching Threads
    else if (thd_id < g_worker_thread_cnt + g_batching_thread_cnt)
    {
        valid = new_txn_queue->pop(entry);
    }
    // Checkpointing Threads
    else if (thd_id < g_worker_thread_cnt + g_batching_thread_cnt + g_checkpointing_thread_cnt)
    {
        valid = checkpoint_queue->pop(entry);
    }
    // Execution Threads
    else if (thd_id < g_worker_thread_cnt + g_batching_thread_cnt + g_checkpointing_thread_cnt + g_execution_thread_cnt)
    {
        uint64_t queue_id = (get_expectedExecuteCount() / get_batch_size()) % indexSize;
        valid = execution_queues[queue_id]->pop(entry);
    }

    if (valid)
    {
        message = entry->msg;
        assert(message);
        uint64_t queue_time = get_sys_clock() - entry->starttime;
        INC_STATS(thd_id, work_queue_wait_time, queue_time);
        INC_STATS(thd_id, work_queue_cnt, 1);

        message->wq_time = queue_time;
        DEBUG("Work Dequeue (%ld,%ld)\n", entry->txn_id, entry->batch_id);
        mem_allocator.free(entry, sizeof(work_queue_entry));
        INC_STATS(thd_id, work_queue_dequeue_time, get_sys_clock() - starttime);
    }

    return message;
}

#endif // ENABLE_PIPELINE == true
