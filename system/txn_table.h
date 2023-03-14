#ifndef _TXN_TABLE_H_
#define _TXN_TABLE_H_

#include "global.h"
#include <map>
#include <utility>

class TxnManager;
class BaseQuery;

struct txn_node
{
    txn_node()
    {
        next = NULL;
        prev = NULL;
    }
    ~txn_node()
    {
    }
    TxnManager *txn_man;
    uint64_t return_id;      // Client ID or Home partition ID
    uint64_t client_startts; // For sequencer
    uint64_t abort_penalty;
    txn_node *next; // Pointer to next node.
    txn_node *prev; // Pointer to previous node.
};
typedef txn_node *txn_node_t;

struct pool_node
{
public:
    txn_node_t head; // Head of the linked list
    txn_node_t tail; // Tail of the linked list

    volatile bool modify;
    uint64_t cnt;
    uint64_t min_ts;
};
typedef pool_node *pool_node_t;

class TxnTable
{
public:
    void init();
    void free();
    TxnManager *get_transaction_manager(uint64_t thd_id, uint64_t txn_id, uint64_t batch_id);
    void release_transaction_manager(uint64_t thd_id, uint64_t txn_id, uint64_t batch_id);
    void update_min_ts(uint64_t thd_id, uint64_t txn_id, uint64_t batch_id, uint64_t ts);
    uint64_t get_min_ts(uint64_t thd_id);
    bool is_matching_txn_node(txn_node_t t_node, uint64_t txn_id, uint64_t batch_id);

private:
    //  TxnMap pool;
    uint64_t pool_size; // Number of link lists
    pool_node **pool;
};

#endif
