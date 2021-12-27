#ifndef _SEMAMAN_H_
#define _SEMAMAN_H_

#include "global.h"
#include <mutex>
#include <semaphore.h>
#include <vector>

enum SpecialSemaphore{
#if CONSENSUS == HOTSTUFF
    NEW_TXN,
#endif
    EXECUTE,
    SETUP_DONE
};

class ExecuteMsgHeap{
public:
    // A min heap storing txn_id of execute_msgs
    std::priority_queue<uint64_t , std::vector<uint64_t>, std::greater<uint64_t> > heap;
    std::mutex heap_lock;
    ExecuteMsgHeap(){}
    void push(uint64_t txn_id);
    uint64_t top();
    void pop();
};

class SemaphoreManager{
public:
    // Entities for semaphore optimizations. The value of the semaphores means 
    // the number of msgs in the corresponding queues of the worker_threads.
    // Only worker_threads with msgs in their queues will be allocated with CPU resources.
    sem_t *worker_queue_semaphore;
#if CONSENSUS == HOTSTUFF
    // new_txn_semaphore is the number of instances that a replica is primary and has not sent a prepare msg.
    sem_t new_txn_semaphore;
#endif
    // execute_semaphore is whether the next msg to execute has been in the queue.
    sem_t execute_semaphore;
    // Entities for semaphore opyimizations on output_thread. The output_thread will be not allocated
    // with CPU resources until there is a msg in its queue.
    sem_t *output_semaphore;
    // Semaphore indicating whether the setup is done
    sem_t setup_done_barrier;

    // The number of INIT_DONE and KEYEX msgs that each output_thread needs to send.
    uint64_t *init_msg_cnt;

    // A min-heap storing the txn_id of execute msgs enqueued
    ExecuteMsgHeap emheap;

    void init();
    void dec_init_msg_cnt(uint64_t td_id);
    uint64_t get_init_msg_cnt(uint64_t td_id);
    void wait(uint32_t thd_id, bool is_output_queue = false, bool is_worker_queue = true);
    void post(uint32_t thd_id, bool is_output_queue = false, bool is_worker_queue = true);
    void post_all();
};
#endif