// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "eEVM/simple/simpleaccount.h"
#include "service/contract/executor/x_manager/concurrency_controller.h"
#include "service/contract/executor/x_manager/db_view.h"
#include "service/contract/executor/x_manager/evm_state.h"

namespace resdb {
namespace contract {
namespace x_manager {

class ExecutorState : public EVMState {
 public:
  using StateEntry = std::pair<eevm::SimpleAccount, DBView>;

 public:
  ExecutorState(ConcurrencyController* controller, int64_t commit_id);
  virtual ~ExecutorState() = default;

  virtual void remove(const eevm::Address& addr) override;

  // Get contract by contract address.
  eevm::AccountState get(const eevm::Address& addr) override;

  bool Exists(const eevm::Address& addr);

  // Flesh the local view to the controller with a commit id.
  // Once all the contracts have fleshed their changes, they should call commit.
  // Return false if contract not exists.
  bool Flesh(const eevm::Address& addr, int commit_id);
  // Commit the changes using the commit id from the flesh.
  // bool Commit(const eevm::Address& addr);

  // Create an account for the contract, which the balance is 0.
  eevm::AccountState create(const eevm::Address& addr, const uint256_t& balance,
                            const eevm::Code& code) override;

  const eevm::SimpleAccount& GetAccount(const eevm::Address& addr);
  void Set(const eevm::SimpleAccount& acc, int64_t commit_id, int version);

 protected:
  void Insert(const StateEntry& p);

 private:
  std::map<eevm::Address, StateEntry> accounts;
  ConcurrencyController* controller_;
};

}  // namespace x_manager
}  // namespace contract
}  // namespace resdb
