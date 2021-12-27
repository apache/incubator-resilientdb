#include "global.h"
#include "sema_manager.h"

void ExecuteMsgHeap::push(uint64_t txn_id){
    heap_lock.lock();
    heap.push(txn_id);
    heap_lock.unlock();
}

uint64_t ExecuteMsgHeap::top(){
    uint64_t value = 0;
	heap_lock.lock();
	value = heap.top();
	heap_lock.unlock();
    return value;
}

void ExecuteMsgHeap::pop(){
    heap_lock.lock();
    heap.pop();
    heap_lock.unlock();
}

void SemaphoreManager::init(){
    worker_queue_semaphore = new sem_t[THREAD_CNT];
    output_semaphore = new sem_t[SEND_THREAD_CNT];
    for(uint i = 0; i < THREAD_CNT; i++){
        sem_init(&worker_queue_semaphore[i], 0, 0);
    }

    for(uint i = 0; i < SEND_THREAD_CNT; i++){
        sem_init(&output_semaphore[i], 0, 0);
    }

    sem_init(&execute_semaphore, 0, 0);
    sem_init(&setup_done_barrier, 0, 0);
    
#if CONSENSUS == HOTSTUFF
    #if !PVP 
        if(g_node_id==0)
            sem_init(&new_txn_semaphore, 0, 1); // Initially, replica 0 is the primary
        else
            sem_init(&new_txn_semaphore, 0, 0);
    #else
        sem_init(&new_txn_semaphore, 0, 1); // Initially, each replica is primary of one instance
    #endif
#endif

    init_msg_cnt = new uint64_t[SEND_THREAD_CNT];
    for(uint i = 0; i < SEND_THREAD_CNT; i++){
        init_msg_cnt[i] = 0;
    }
    for(uint i = 0; i < g_total_node_cnt; i++){
        if(i == g_node_id)
            continue;
        init_msg_cnt[i%SEND_THREAD_CNT] += 3;
    }
}


void SemaphoreManager::wait(uint32_t thd_id, bool is_output_queue, bool is_worker_queue){
    if(is_output_queue){
        sem_wait(&output_semaphore[thd_id]);
    }
    else if(is_worker_queue){
        // Wait until there is at least one msg in its corresponding queue
        sem_wait(&worker_queue_semaphore[thd_id]);
#if CONSENSUS == HOTSTUFF
        // The batch_thread waits until the replica becomes primary of at least one instance 
        if(thd_id == g_thread_cnt - 1 - g_checkpointing_thread_cnt - g_execution_thread_cnt)   
            sem_wait(&new_txn_semaphore);
#endif
        // The execute_thread waits until the next msg to execute is enqueued
        if (thd_id == g_thread_cnt - 1)  
            sem_wait(&execute_semaphore);
    }
    else{
        switch(thd_id){
            case SETUP_DONE:{
                sem_wait(&setup_done_barrier);
                break;
            }
            default:{
                assert(0);
            }
        }
    }
}

void SemaphoreManager::post(uint32_t thd_id, bool is_output_queue, bool is_worker_queue){
    if(is_output_queue){
        sem_post(&output_semaphore[thd_id]);
    }
    else if(is_worker_queue){
        sem_post(&worker_queue_semaphore[thd_id]);
    }else{
        switch(thd_id){
#if CONSENSUS == HOTSTUFF
            case NEW_TXN:{
                sem_post(&new_txn_semaphore);
                break;
            }
#endif
            case EXECUTE:{
                sem_post(&execute_semaphore);
                break;
            }
            case SETUP_DONE:{
                sem_post(&setup_done_barrier);
                break;
            }
            default:{
                assert(0);
            }
        }
    }
}

void SemaphoreManager::post_all(){
    // After simulation is done, sem_post all sempahores
    // Otherwise, some threads may be stalled by sem_wait()
    for(uint i=0; i<g_thread_cnt; i++){
        sem_post(&worker_queue_semaphore[i]);
    }
    for(uint i=0; i<g_this_send_thread_cnt; i++){
        sem_post(&output_semaphore[i]);
    }
#if CONSENSUS == HOTSTUFF
    sem_post(&new_txn_semaphore);
#endif
    sem_post(&execute_semaphore);
}

void SemaphoreManager::dec_init_msg_cnt(uint64_t td_id){
    init_msg_cnt[td_id]--;
}
    
uint64_t SemaphoreManager::get_init_msg_cnt(uint64_t td_id){
    return init_msg_cnt[td_id];
}
