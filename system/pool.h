#ifndef _TXN_POOL_H_
#define _TXN_POOL_H_

#include "global.h"
#include <boost/lockfree/queue.hpp>
#include <mutex>

class TxnManager;
class BaseQuery;
class Workload;
struct msg_entry;
struct txn_node;
class Transaction;

class TxnManPool
{
public:
    void init(Workload *wl, uint64_t size);
    void get(uint64_t txn_id, TxnManager *&item);
    void put(uint64_t txn_id, TxnManager *items);
    void free_all();

private:
    boost::lockfree::queue<TxnManager *> **pool; // Pools for txn managers.
    Workload *_wl;
};

class TxnPool
{
public:
    void init(Workload *wl, uint64_t size);
    void get(uint64_t pool_id, Transaction *&item);
    void put(uint64_t pool_id, Transaction *items);
    void free_all();

private:
    boost::lockfree::queue<Transaction *> **pool;
    Workload *_wl;
};

class QryPool
{
public:
    void init(Workload *wl, uint64_t size);
    void get(uint64_t pool_id, BaseQuery *&item);
    void put(uint64_t pool_id, BaseQuery *items);
    void free_all();

private:
    boost::lockfree::queue<BaseQuery *> **pool;
    Workload *_wl;
};

class TxnTablePool
{
public:
    void init(Workload *wl, uint64_t size);
    void get(uint64_t txn_id, txn_node *&item);
    void put(uint64_t txn_id, txn_node *items);
    void free_all();

private:
    boost::lockfree::queue<txn_node *> **pool;
    Workload *_wl;
};

#endif
