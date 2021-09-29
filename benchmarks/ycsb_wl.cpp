#include "global.h"
#include "ycsb.h"
#include "wl.h"
#include "thread.h"
#include "mem_alloc.h"
#include "query.h"

#if !BANKING_SMART_CONTRACT
int YCSBWorkload::next_tid;

RC YCSBWorkload::init()
{
  Workload::init();
  next_tid = 0;
  return RCOK;
}

int YCSBWorkload::key_to_part(uint64_t key)
{
  return key % g_part_cnt;
}

RC YCSBWorkload::get_txn_man(TxnManager *&txn_manager)
{
  DEBUG_M("YCSBWorkload::get_txn_man YCSBTxnManager alloc\n");
  txn_manager = (YCSBTxnManager *)
                    mem_allocator.align_alloc(sizeof(YCSBTxnManager));
  new (txn_manager) YCSBTxnManager();
  return RCOK;
}
#endif
