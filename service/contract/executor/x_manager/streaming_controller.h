#pragma once

#include <deque>
#include <set>
#include <shared_mutex>
#include <thread>

#include "service/contract/executor/x_manager/concurrency_controller.h"
#include "service/contract/executor/x_manager/d_storage.h"

namespace resdb {
namespace contract {
namespace x_manager {

class StreamingController : public ConcurrencyController {
 public:
  StreamingController(DataStorage* storage);
  virtual ~StreamingController();

  // ==============================
  virtual void Store(const int64_t commit_id, const uint256_t& key,
                     const uint256_t& value, int version);
  virtual uint256_t Load(const int64_t commit_id, const uint256_t& key,
                         int version);
  bool Remove(const int64_t commit_id, const uint256_t& key, int version);

  void PushCommit(int64_t commit_id, const ModifyMap& local_changes_) override {
  }
  // std::vector<std::unique_ptr<ExecuteResp>>
  // ExecContract(std::vector<ContractExecuteInfo>& request) override { return
  // std::vector<std::unique_ptr<ExecuteResp>>(); }

 private:
  DataStorage* storage_;
};

}  // namespace x_manager
}  // namespace contract
}  // namespace resdb
