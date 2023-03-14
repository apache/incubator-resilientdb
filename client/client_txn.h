#ifndef _CLIENT_TXN_H_
#define _CLIENT_TXN_H_

#include "global.h"
#include "semaphore.h"

class Inflight_entry
{
public:
    void init();
    int32_t inc_inflight();
    int32_t dec_inflight();
    int32_t get_inflight();

private:
    volatile int32_t num_inflight_txns;
    sem_t mutex;
};

class Client_txn
{
public:
    void init();
    int32_t inc_inflight(uint32_t node_id);
    int32_t dec_inflight(uint32_t node_id);
    int32_t get_inflight(uint32_t node_id);

private:
    Inflight_entry **inflight_txns;
};

#endif
