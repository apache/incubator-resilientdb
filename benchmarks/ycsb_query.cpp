#include "query.h"
#include "ycsb_query.h"
#include "mem_alloc.h"
//#include "wl.h"
#include "ycsb.h"
#include "helper.h"
#include "message.h"

uint64_t YCSBQueryGenerator::the_n = 0;
double YCSBQueryGenerator::denom = 0;

void YCSBQueryGenerator::init()
{
    mrand = (myrand *)mem_allocator.alloc(sizeof(myrand));
    mrand->init(get_sys_clock());

    zeta_2_theta = zeta(2, g_zipf_theta);
    uint64_t table_size = g_synth_table_size / g_part_cnt;
    the_n = table_size - 1;
    denom = zeta(the_n, g_zipf_theta);
}

BaseQuery *YCSBQueryGenerator::create_query()
{
    BaseQuery *query;
    assert(the_n != 0);
    query = gen_requests_zipf();

    return query;
}

void YCSBQuery::print() {}

void YCSBQuery::init()
{
    requests.init(g_req_per_query);
}

void YCSBQuery::copy_request_to_msg(YCSBQuery *ycsb_query, YCSBQueryMessage *msg, uint64_t id)
{
    msg->requests.add(ycsb_query->requests[id]);
}

void YCSBQuery::release_requests()
{
    for (uint64_t i = 0; i < requests.size(); i++)
    {
        DEBUG_M("YCSBQuery::release() ycsb_request free\n");
        mem_allocator.free(requests[i], sizeof(ycsb_request));
    }
}

void YCSBQuery::reset()
{
    release_requests();
    requests.clear();
}

void YCSBQuery::release()
{
    DEBUG_M("YCSBQuery::release() free\n");
    release_requests();
    requests.release();
}

// The following algorithm comes from the paper:
// Quickly generating billion-record synthetic databases
// However, it seems there is a small bug.
// The original paper says zeta(theta, 2.0). But I guess it should be
// zeta(2.0, theta).
double YCSBQueryGenerator::zeta(uint64_t n, double theta)
{
    double sum = 0;
    for (uint64_t i = 1; i <= n; i++)
        sum += pow(1.0 / i, theta);
    return sum;
}

uint64_t YCSBQueryGenerator::zipf(uint64_t n, double theta)
{
    assert(this->the_n == n);
    assert(theta == g_zipf_theta);
    double alpha = 1 / (1 - theta);
    double zetan = denom;
    double eta = (1 - pow(2.0 / n, 1 - theta)) /
                 (1 - zeta_2_theta / zetan);
    //	double eta = (1 - pow(2.0 / n, 1 - theta)) /
    //		(1 - zeta_2_theta / zetan);
    double u = (double)(mrand->next() % 10000000) / 10000000;
    double uz = u * zetan;
    if (uz < 1)
        return 1;
    if (uz < 1 + pow(0.5, theta))
        return 2;
    return 1 + (uint64_t)(n * pow(eta * u - eta + 1, alpha));
}

BaseQuery *YCSBQueryGenerator::gen_requests_zipf()
{
    YCSBQuery *query = (YCSBQuery *)mem_allocator.alloc(sizeof(YCSBQuery));
    new (query) YCSBQuery();
    query->requests.init(g_req_per_query);

    uint64_t table_size = g_synth_table_size;

    int rid = 0;
    for (UInt32 i = 0; i < g_req_per_query; i++)
    {
        //double r = (double)(mrand->next() % 10000) / 10000;
        ycsb_request *req = (ycsb_request *)mem_allocator.alloc(sizeof(ycsb_request));
        uint64_t row_id = zipf(table_size - 1, g_zipf_theta);
        ;
        assert(row_id < table_size);

        req->key = row_id;
        req->value = mrand->next();

        rid++;

        query->requests.add(req);
    }
    assert(query->requests.size() == g_req_per_query);

    // Sort the requests in key order.
    if (g_key_order)
    {
        for (uint64_t i = 0; i < query->requests.size(); i++)
        {
            for (uint64_t j = query->requests.size() - 1; j > i; j--)
            {
                if (query->requests[j]->key < query->requests[j - 1]->key)
                {
                    query->requests.swap(j, j - 1);
                }
            }
        }
    }

    query->print();
    return query;
}
