#pragma once

#include <map>

#include "service/contract/executor/x_manager/concurrency_controller.h"
#include "service/contract/executor/x_manager/streaming_e_controller.h"
#include "eEVM/storage.h"

namespace resdb {
namespace contract {
namespace x_manager {

class DBView : public eevm::Storage {

public:
  DBView(ConcurrencyController * controller, int64_t commit_id, int version);
  virtual ~DBView() = default;

  void store(const uint256_t& key, const uint256_t& value) override;
  uint256_t load(const uint256_t& key) override;
  bool remove(const uint256_t& key) override;

  // for 2PL, once it is done, all the commit will be pushed to
  // the controller to judge if it can be committed.
  // During the flesh, all the changes will be removed.
  void Flesh(int64_t commit_id) {}
  // Commit the changes. If there is a conflict, return false.
  // Make sure all other committers have pushed their changes before calling Commit.
  //bool Commit();
  // Remove all the changes.
  //void Abort();

private:
  StreamingEController * controller_;
  int64_t commit_id_;
  int version_;
  std::map<uint256_t, std::vector<Data>> local_changes_;
};

}
}
} // namespace resdb
