#ifndef _QUERY_H_
#define _QUERY_H_

#include "global.h"
#include "array.h"

class Workload;
class YCSBQuery;

class BaseQuery
{
public:
    virtual ~BaseQuery() {}
    virtual void print() = 0;
    virtual void init() = 0;
    uint64_t waiting_time;
    //void clear() = 0;
    virtual void release() = 0;
};

class QueryGenerator
{
public:
    virtual ~QueryGenerator() {}
    virtual BaseQuery *create_query() = 0;
};

// All the queries for a particular thread.
class Query_thd
{
public:
    void init(Workload *h_wl, int thread_id);
    BaseQuery *get_next_query();
    int q_idx;
    YCSBQuery *queries;

    char pad[CL_SIZE - sizeof(void *) - sizeof(int)];
};

class Query_queue
{
public:
    void init(Workload *h_wl);
    void init(int thread_id);
    BaseQuery *get_next_query(uint64_t thd_id);

private:
    Query_thd **all_queries;
    Workload *_wl;
};

#endif
