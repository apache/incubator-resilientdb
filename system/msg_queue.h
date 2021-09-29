#ifndef _MSG_QUEUE_H_
#define _MSG_QUEUE_H_

#include "global.h"
#include "lock_free_queue.h"
#include <boost/lockfree/queue.hpp>

class BaseQuery;
class Message;

class msg_entry
{
public:
    Message *msg;
    uint64_t starttime;
    vector<string> allsign;
};

typedef msg_entry *msg_entry_t;

class MessageQueue
{
public:
    void init();

    void enqueue(uint64_t thd_id, Message *msg, const vector<uint64_t> &dest);
    vector<uint64_t> dequeue(uint64_t thd_id, vector<string> &allsign, Message *&msg);

private:
// This is close to max capacity for boost
#if NETWORK_DELAY_TEST
    boost::lockfree::queue<msg_entry *> **cl_m_queue;
#endif
    boost::lockfree::queue<msg_entry *> **m_queue;
    std::vector<msg_entry *> sthd_m_cache;
    uint64_t **ctr;
};

#endif
