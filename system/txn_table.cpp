#include "global.h"
#include "txn_table.h"
#include "ycsb_query.h"
#include "ycsb.h"
#include "query.h"
#include "txn.h"
#include "mem_alloc.h"
#include "pool.h"
#include "work_queue.h"
#include "message.h"

void TxnTable::init()
{
    DEBUG_M("TxnTable::init pool_node alloc\n");
    pool_size = indexSize + 1;
    pool = (pool_node **)mem_allocator.align_alloc(sizeof(pool_node *) * pool_size);
    for (uint32_t i = 0; i < pool_size; i++)
    {
        pool[i] = (pool_node *)mem_allocator.align_alloc(sizeof(struct pool_node));
        pool[i]->head = NULL;
        pool[i]->tail = NULL;
        pool[i]->cnt = 0;
        pool[i]->modify = false;
        pool[i]->min_ts = UINT64_MAX;
    }
}

void TxnTable::free()
{
    for (uint32_t i = 0; i < pool_size; i++)
    {
        mem_allocator.free(pool[i], 0);
    }
    mem_allocator.free(pool, 0);
}

bool TxnTable::is_matching_txn_node(txn_node_t t_node, uint64_t txn_id, uint64_t batch_id)
{
    assert(t_node);
    return (t_node->txn_man->get_txn_id() == txn_id && t_node->txn_man->get_batch_id() == batch_id);
}

void TxnTable::update_min_ts(uint64_t thd_id, uint64_t txn_id, uint64_t batch_id, uint64_t ts)
{

    uint64_t pool_id = txn_id % pool_size;
    while (!ATOM_CAS(pool[pool_id]->modify, false, true))
    {
    };
    if (ts < pool[pool_id]->min_ts)
        pool[pool_id]->min_ts = ts;
    ATOM_CAS(pool[pool_id]->modify, true, false);
}

uint64_t TxnTable::get_min_ts(uint64_t thd_id)
{
    uint64_t min_ts = UINT64_MAX;
    for (uint64_t i = 0; i < pool_size; i++)
    {
        uint64_t pool_min_ts = pool[i]->min_ts;
        if (pool_min_ts < min_ts)
            min_ts = pool_min_ts;
    }

    return min_ts;
}

/*
    This function creates a new txn manager, if not present, otherwise fetches an existing txn manager.
*/
TxnManager *TxnTable::get_transaction_manager(uint64_t thd_id, uint64_t txn_id, uint64_t batch_id)
{
    uint64_t starttime = get_sys_clock();

    // Selecting the link list to fetch (or put) the transaction.
    uint64_t pool_id = txn_id % pool_size;
    DEBUG_Q("TxnTable::get_txn_manager, n_%u, thd_id=%lu, txn_id=%lu, pool_id=%lu, pool_size=%lu\n",
            g_node_id, thd_id, txn_id, pool_id, pool_size);

    TxnManager *txn_man = NULL;

    // set modify bit for this pool: Lock
    while (!ATOM_CAS(pool[pool_id]->modify, false, true))
    {
    };

    txn_node_t t_node = pool[pool_id]->head;
    while (t_node != NULL)
    {
        if (is_matching_txn_node(t_node, txn_id, batch_id))
        {
            // Transaction manager found.
            txn_man = t_node->txn_man;
            break;
        }
        t_node = t_node->next;
    }

    if (txn_man)
    {
        // unset modify bit for this pool: Unlock
        ATOM_CAS(pool[pool_id]->modify, true, false);
    }
    else
    {

        // Allocate memory for a txn_node.
        txn_table_pool.get(txn_id, t_node);

        // Allocate memory for a txn manager.
        txn_man_pool.get(txn_id, txn_man);

        // Set fields for txn manager.
        txn_man->set_txn_id(txn_id);
        txn_man->set_batch_id(batch_id);

        t_node->txn_man = txn_man;
        txn_man->txn_stats.starttime = get_sys_clock();
        txn_man->txn_stats.restart_starttime = txn_man->txn_stats.starttime;

        // Put the txn manager in the list.
        LIST_PUT_TAIL(pool[pool_id]->head, pool[pool_id]->tail, t_node);

        ++pool[pool_id]->cnt;
        INC_STATS(thd_id, txn_table_new_cnt, 1);

        // unset modify bit for this pool: Unlock.
        ATOM_CAS(pool[pool_id]->modify, true, false);
    }

    INC_STATS(thd_id, txn_table_get_time, get_sys_clock() - starttime);
    INC_STATS(thd_id, txn_table_get_cnt, 1);
    return txn_man;
}

void TxnTable::release_transaction_manager(uint64_t thd_id, uint64_t txn_id, uint64_t batch_id)
{
    uint64_t starttime = get_sys_clock();
    DEBUG_Q("release txm_mgr: thd_id=%lu, txn_id=%lu, batch_id=%lu\n", thd_id, txn_id, batch_id);
    uint64_t pool_id = txn_id % pool_size;

    // Lock the pool before access.
    // set modify bit for this pool: txn_id % pool_size
    while (!ATOM_CAS(pool[pool_id]->modify, false, true))
    {
    };

    txn_node_t t_node = pool[pool_id]->head;

    // Check if matching txn manager found in the global pool.
    while (t_node != NULL)
    {
        if (is_matching_txn_node(t_node, txn_id, batch_id))
        {
            LIST_REMOVE_HT(t_node, pool[txn_id % pool_size]->head, pool[txn_id % pool_size]->tail);
            --pool[pool_id]->cnt;
            break;
        }
        t_node = t_node->next;
    }

    // Unlock the global pool.
    // unset modify bit for this pool: txn_id % pool_size
    ATOM_CAS(pool[pool_id]->modify, true, false);

    //assert(t_node);
    if (t_node == NULL)
    {
        cout << "txn: " << txn_id;
        assert(0);
    }
    assert(t_node->txn_man);

    // Releasing the txn manager.
    txn_man_pool.put(txn_id, t_node->txn_man);

    // Releasing the node associated with the txn_mann
    txn_table_pool.put(txn_id, t_node);

    INC_STATS(thd_id, txn_table_release_time, get_sys_clock() - starttime);
    INC_STATS(thd_id, txn_table_release_cnt, 1);
}
