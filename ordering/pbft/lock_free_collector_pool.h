#pragma once

#include <vector>

#include "ordering/pbft/transaction_collector.h"

namespace resdb {

class LockFreeCollectorPool {
 public:
  LockFreeCollectorPool(const std::string& name, uint32_t size,
                        TransactionExecutor* executor,
                        bool enable_viewchange = false);

  TransactionCollector* GetCollector(uint64_t seq);
  void Update(uint64_t seq);

 private:
  std::string name_;
  uint32_t capacity_;
  uint32_t mask_;
  TransactionExecutor* executor_;
  std::vector<std::unique_ptr<TransactionCollector>> collector_;
  bool enable_viewchange_;
};

}  // namespace resdb
