
#include "query.h"
#include "mem_alloc.h"
#include "wl.h"
#include "ycsb_query.h"

/*************************************************/
//     class Query_queue
/*************************************************/

void Query_queue::init(Workload *h_wl)
{
	all_queries = new Query_thd *[g_thread_cnt];
	_wl = h_wl;
	for (UInt32 tid = 0; tid < g_thread_cnt; tid++)
		init(tid);
}

void Query_queue::init(int thread_id)
{
	all_queries[thread_id] = (Query_thd *)mem_allocator.alloc(sizeof(Query_thd));
	all_queries[thread_id]->init(_wl, thread_id);
}

BaseQuery *Query_queue::get_next_query(uint64_t thd_id)
{
	BaseQuery *query = all_queries[thd_id]->get_next_query();
	return query;
}

void Query_thd::init(Workload *h_wl, int thread_id)
{
	uint64_t request_cnt;
	q_idx = 0;
	request_cnt = WARMUP / g_thread_cnt + MAX_TXN_PER_PART + 4;
	queries = (YCSBQuery *)
				  mem_allocator.alloc(sizeof(YCSBQuery) * request_cnt);

	for (UInt32 qid = 0; qid < request_cnt; qid++)
	{
		new (&queries[qid]) YCSBQuery();
		queries[qid].init(thread_id, h_wl);
	}
}

BaseQuery *Query_thd::get_next_query()
{
	BaseQuery *query = &queries[q_idx++];
	return query;
}
