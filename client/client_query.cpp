#include "client_query.h"
#include "mem_alloc.h"
#include "ycsb_query.h"

/*************************************************/
//     class Query_queue
/*************************************************/

void Client_query_queue::init()
{
    std::vector<BaseQuery *> new_queries(g_max_txn_per_part + 4, NULL);
    queries = new_queries;
    query_cnt = (uint64_t *)mem_allocator.align_alloc(sizeof(uint64_t));
    next_tid = 0;

    // Parallel creation of queries.
    pthread_t *p_thds = new pthread_t[g_init_parallelism - 1];
    for (UInt32 i = 0; i < g_init_parallelism - 1; i++)
    {
        pthread_create(&p_thds[i], NULL, initQueriesHelper, this);
        pthread_setname_np(p_thds[i], "clientquery");
    }

    initQueriesHelper(this);

    for (uint32_t i = 0; i < g_init_parallelism - 1; i++)
    {
        pthread_join(p_thds[i], NULL);
    }
}

void *Client_query_queue::initQueriesHelper(void *context)
{
    ((Client_query_queue *)context)->initQueriesParallel();
    return NULL;
}

void Client_query_queue::initQueriesParallel()
{
    UInt32 tid = ATOM_FETCH_ADD(next_tid, 1);
    uint64_t request_cnt = g_max_txn_per_part + 4;
    uint32_t final_request;

    if (tid == g_init_parallelism - 1)
    {
        final_request = request_cnt;
    }
    else
    {
        final_request = request_cnt / g_init_parallelism * (tid + 1);
    }

    YCSBQueryGenerator *gen = new YCSBQueryGenerator;
    gen->init();

    UInt32 gq_cnt = 0;
    for (UInt32 query_id = request_cnt / g_init_parallelism * tid; query_id < final_request; query_id++)
    {
        queries[query_id] = gen->create_query();
        gq_cnt++;
    }

    DEBUG_WL("final_request = %d\n", final_request)
    DEBUG_WL("request_cnt = %lu\n", request_cnt)
    DEBUG_WL("g_init_parallelism = %d\n", g_init_parallelism)
    DEBUG_WL("Client: tid(%d): generated query count = %d\n", tid, gq_cnt);
}

bool Client_query_queue::done()
{
    return false;
}

BaseQuery *Client_query_queue::get_next_query(uint64_t thread_id)
{
    uint64_t query_id = __sync_fetch_and_add(query_cnt, 1);
    if (query_id > g_max_txn_per_part)
    {
        __sync_bool_compare_and_swap(query_cnt, query_id + 1, 0);
        query_id = __sync_fetch_and_add(query_cnt, 1);
    }

    BaseQuery *query = queries[query_id];
    return query;
}
