#ifndef _CLIENT_QUERY_H_
#define _CLIENT_QUERY_H_

#include "global.h"
#include "query.h"

//class Workload;
class YCSBQuery;
class YCSBClientQuery;

// We assume a separate task queue for each thread in order to avoid
// contention in a centralized query queue.
class Client_query_queue
{
public:
    void init(); //Workload * h_wl);
    bool done();
    BaseQuery *get_next_query(uint64_t thread_id);
    void initQueriesParallel();
    static void *initQueriesHelper(void *context);

private:
    //Workload * _wl;
    uint64_t size;
    std::vector<BaseQuery *> queries;
    uint64_t *query_cnt;
    volatile uint64_t next_tid;
};

#endif
