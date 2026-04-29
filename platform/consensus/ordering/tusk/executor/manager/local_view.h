#pragma once

#include <map>

#include "service/contract/executor/manager/concurrency_controller.h"
#include "eEVM/storage.h"

namespace resdb {
namespace contract {

class LocalView : public eevm::Storage {

public:
  LocalView(ConcurrencyController * controller, int64_t commit_id);
  virtual ~LocalView() = default;

  void store(const uint256_t& key, const uint256_t& value) override;
  uint256_t load(const uint256_t& key) override;
  bool remove(const uint256_t& key) override;

  // for 2PL, once it is done, all the commit will be pushed to
  // the controller to judge if it can be committed.
  // During the flesh, all the changes will be removed.
  void Flesh(int64_t commit_id);
  // Commit the changes. If there is a conflict, return false.
  // Make sure all other committers have pushed their changes before calling Commit.
  //bool Commit();
  // Remove all the changes.
  //void Abort();

private:
  ConcurrencyController * controller_;
  int64_t commit_id_;
  std::map<uint256_t, std::vector<Data>> local_changes_;
};

}
} // namespace resdb
