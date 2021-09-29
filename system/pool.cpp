#include "pool.h"
#include "global.h"
#include "txn.h"
#include "mem_alloc.h"
#include "wl.h"
#include "ycsb_query.h"
#include "ycsb.h"
#include "query.h"
#include "msg_queue.h"

#define TRY_LIMIT 10

/*
We initialize the pools for storing transaction manager. 
The total number of such pools is equal to total threads, with an assumtion of providing parallelism equivalent to number of threads.
*/
void TxnManPool::init(Workload *wl, uint64_t size)
{
    _wl = wl;

    pool = new boost::lockfree::queue<TxnManager *> *[g_total_thread_cnt];
    TxnManager *txn;
    for (uint64_t thd_id = 0; thd_id < g_total_thread_cnt; thd_id++)
    {
        pool[thd_id] = new boost::lockfree::queue<TxnManager *>(size);

        for (uint64_t i = 0; i < size; i++)
        {
            _wl->get_txn_man(txn);
            txn->init(thd_id, _wl);
            put(thd_id, txn);
        }
    }
}

/* Fetches a txn manager and allocates, if unavailable. */
void TxnManPool::get(uint64_t txn_id, TxnManager *&item)
{
    uint64_t pool_id = txn_id % g_total_thread_cnt;
    bool r = pool[pool_id]->pop(item);
    if (!r)
    {
        _wl->get_txn_man(item);
    }
    item->init(pool_id, _wl);
}

/* Puts a txn manager (item) back into pool. */
void TxnManPool::put(uint64_t txn_id, TxnManager *item)
{
    uint64_t pool_id = txn_id % g_total_thread_cnt;
    item->release(pool_id);
    int tries = 0;
    while (!pool[pool_id]->push(item) && tries++ < TRY_LIMIT)
    {
    }
    if (tries >= TRY_LIMIT)
    {
        mem_allocator.free(item, sizeof(TxnManager));
        // Delete
        delete item;
    }
}

void TxnManPool::free_all()
{
    TxnManager *item;
    for (uint64_t thd_id = 0; thd_id < g_total_thread_cnt; thd_id++)
    {
        while (pool[thd_id]->pop(item))
        {
            mem_allocator.free(item, sizeof(TxnManager));
            // Delete
            delete item;
        }
    }
}

void TxnPool::init(Workload *wl, uint64_t size)
{
    _wl = wl;

    pool = new boost::lockfree::queue<Transaction *> *[g_total_thread_cnt];
    Transaction *txn;
    for (uint64_t thd_id = 0; thd_id < g_total_thread_cnt; thd_id++)
    {
        pool[thd_id] = new boost::lockfree::queue<Transaction *>(size);
        for (uint64_t i = 0; i < size; i++)
        {
            txn = (Transaction *)mem_allocator.alloc(sizeof(Transaction));
            txn->init();
            put(thd_id, txn);
        }
    }
}

void TxnPool::get(uint64_t pool_id, Transaction *&item)
{
    bool r = pool[pool_id]->pop(item);
    if (!r)
    {
        item = (Transaction *)mem_allocator.alloc(sizeof(Transaction));
        item->init();

        //cout << "alloc txn " << pool_id << "\n";
        //fflush(stdout);
    }
}

void TxnPool::put(uint64_t pool_id, Transaction *item)
{
    item->reset(pool_id);
    int tries = 0;

    while (!pool[pool_id]->push(item) && tries++ < TRY_LIMIT)
    {
    }
    if (tries >= TRY_LIMIT)
    {
        item->release(pool_id);
        mem_allocator.free(item, sizeof(Transaction));
    }
}

void TxnPool::free_all()
{
    Transaction *item;
    for (uint64_t thd_id = 0; thd_id < g_total_thread_cnt; thd_id++)
    {
        while (pool[thd_id]->pop(item))
        {
            mem_allocator.free(item, sizeof(Transaction));
        }
    }
}

void QryPool::init(Workload *wl, uint64_t size)
{
    _wl = wl;

    pool = new boost::lockfree::queue<BaseQuery *> *[g_total_thread_cnt];
    BaseQuery *qry = NULL;
    DEBUG_M("QryPool alloc init\n");
    for (uint64_t thd_id = 0; thd_id < g_total_thread_cnt; thd_id++)
    {
        pool[thd_id] = new boost::lockfree::queue<BaseQuery *>(size);
        for (uint64_t i = 0; i < size; i++)
        {
            YCSBQuery *m_qry = (YCSBQuery *)mem_allocator.alloc(sizeof(YCSBQuery));
            m_qry = new YCSBQuery();
            m_qry->init();
            qry = m_qry;
            put(thd_id, qry);
        }
    }
}

void QryPool::get(uint64_t pool_id, BaseQuery *&item)
{
    bool r = pool[pool_id]->pop(item);
    if (!r)
    {
        YCSBQuery *qry = NULL;
        qry = (YCSBQuery *)mem_allocator.alloc(sizeof(YCSBQuery));
        new (qry) YCSBQuery();
        qry->init();
        item = (BaseQuery *)qry;

        //cout << "alloc qry " << pool_id << "\n";
        //fflush(stdout);
    }
    DEBUG_R("get 0x%lx\n", (uint64_t)item);
}

void QryPool::put(uint64_t pool_id, BaseQuery *item)
{
    assert(item);
    ((YCSBQuery *)item)->release();
    DEBUG_R("put 0x%lx\n", (uint64_t)item);
    int tries = 0;

    while (!pool[pool_id]->push(item) && tries++ < TRY_LIMIT)
    {
    }
    if (tries >= TRY_LIMIT)
    {
        ((YCSBQuery *)item)->release();
        mem_allocator.free(item, sizeof(YCSBQuery));
        delete item;
    }
}

void QryPool::free_all()
{
    BaseQuery *item;
    DEBUG_M("query_pool free\n");
    for (uint64_t thd_id = 0; thd_id < g_total_thread_cnt; thd_id++)
    {
        while (pool[thd_id]->pop(item))
        {
            ((YCSBQuery *)item)->release();
            mem_allocator.free(item, sizeof(YCSBQuery));
            delete item;
        }
    }
}

void TxnTablePool::init(Workload *wl, uint64_t size)
{
    _wl = wl;
    pool = new boost::lockfree::queue<txn_node *> *[g_total_thread_cnt];
    DEBUG_M("TxnTablePool alloc init\n");
    for (uint64_t thd_id = 0; thd_id < g_total_thread_cnt; thd_id++)
    {
        pool[thd_id] = new boost::lockfree::queue<txn_node *>(size);
        for (uint64_t i = 0; i < size; i++)
        {
            txn_node *t_node = (txn_node *)mem_allocator.align_alloc(sizeof(struct txn_node));
            put(thd_id, t_node);
        }
    }
}

void TxnTablePool::get(uint64_t txn_id, txn_node *&item)
{
    uint64_t pool_id = txn_id % g_total_thread_cnt;
    bool r = pool[pool_id]->pop(item);
    if (!r)
    {
        DEBUG_M("txn_table_pool alloc\n");
        item = (txn_node *)mem_allocator.align_alloc(sizeof(struct txn_node));
    }
}

void TxnTablePool::put(uint64_t txn_id, txn_node *item)
{
    int tries = 0;
    uint64_t pool_id = txn_id % g_total_thread_cnt;
    while (!pool[pool_id]->push(item) && tries++ < TRY_LIMIT)
    {
    }
    if (tries >= TRY_LIMIT)
    {
        mem_allocator.free(item, sizeof(txn_node));
    }
}

void TxnTablePool::free_all()
{
    txn_node *item;
    DEBUG_M("txn_table_pool free\n");
    for (uint64_t thd_id = 0; thd_id < g_total_thread_cnt; thd_id++)
    {
        while (pool[thd_id]->pop(item))
        {
            mem_allocator.free(item, sizeof(item));
        }
    }
}
