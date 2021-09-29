#ifndef _WORK_QUEUE_H_
#define _WORK_QUEUE_H_

#include "global.h"
#include <queue>
#include <boost/lockfree/queue.hpp>

class BaseQuery;
class Workload;
class Message;

struct work_queue_entry
{
    Message *msg;
    uint64_t batch_id;
    uint64_t txn_id;
    RemReqType rtype;
    uint64_t starttime;
};



class QWorkQueue
{
public:
    void init();
    void enqueue(uint64_t thd_id, Message *msg, bool busy);
    Message *dequeue(uint64_t thd_id);
    void push_to_queue(work_queue_entry* entry, boost::lockfree::queue<work_queue_entry *> *queue);

private:
    boost::lockfree::queue<work_queue_entry *> **execution_queues;
    boost::lockfree::queue<work_queue_entry *> *work_queue;
    boost::lockfree::queue<work_queue_entry *> *new_txn_queue;
    boost::lockfree::queue<work_queue_entry *> *checkpoint_queue;

};

#endif
