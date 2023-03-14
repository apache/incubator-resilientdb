#include "txn.h"
#include "wl.h"
#include "query.h"
#include "thread.h"
#include "mem_alloc.h"
#include "msg_queue.h"
#include "pool.h"
#include "message.h"
#include "ycsb_query.h"
#include "array.h"

#if TESTING_ON

void TxnManager::release_all_messages(uint64_t txn_id)
{
	return;
}

#endif
