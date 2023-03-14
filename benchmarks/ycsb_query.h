#ifndef _YCSBQuery_H_
#define _YCSBQuery_H_

#include "global.h"
#include "query.h"
#include "array.h"

class Workload;
class Message;
class YCSBQueryMessage;
class YCSBClientQueryMessage;

// Each YCSBQuery contains several ycsb_requests,
// to a single table
class ycsb_request
{
public:
    ycsb_request() {}
    ycsb_request(const ycsb_request &req) : key(req.key), value(req.value) {}
    void copy(ycsb_request *req)
    {
        this->key = req->key;
        this->value = req->value;
    }

    uint64_t key;
    uint64_t value;
};

class YCSBQueryGenerator : public QueryGenerator
{
public:
    void init();
    BaseQuery *create_query();

private:
    BaseQuery *gen_requests_zipf();

    // for Zipfian distribution
    double zeta(uint64_t n, double theta);
    uint64_t zipf(uint64_t n, double theta);

    myrand *mrand;
    static uint64_t the_n;
    static double denom;
    double zeta_2_theta;
};

class YCSBQuery : public BaseQuery
{
public:
    YCSBQuery()
    {
    }
    ~YCSBQuery()
    {
    }

    void print();

    void init(uint64_t thd_id, Workload *h_wl){};
    void init();
    void release();
    void release_requests();
    void reset();
    static void copy_request_to_msg(YCSBQuery *ycsb_query, YCSBQueryMessage *msg, uint64_t id);

    Array<ycsb_request *> requests;
};

#endif
