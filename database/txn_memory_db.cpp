#include "database/txn_memory_db.h"

#include <glog/logging.h>

namespace resdb {

TxnMemoryDB::TxnMemoryDB() : max_seq_(0) {}

Request* TxnMemoryDB::Get(uint64_t seq) {
  std::unique_lock<std::mutex> lk(mutex_);
  if (data_.find(seq) == data_.end()) {
    return nullptr;
  }
  return data_[seq].get();
}

void TxnMemoryDB::Put(std::unique_ptr<Request> request) {
  std::unique_lock<std::mutex> lk(mutex_);
  max_seq_ = request->seq();
  data_[max_seq_] = std::move(request);
}

uint64_t TxnMemoryDB::GetMaxSeq() { return max_seq_; }

}  // namespace resdb
