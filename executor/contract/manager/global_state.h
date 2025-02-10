// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "eEVM/simple/simpleaccount.h"
#include "executor/contract/manager/evm_state.h"
#include "executor/contract/manager/global_view.h"

namespace resdb {
namespace contract {

class GlobalState : public EVMState {
 public:
  using StateEntry = std::pair<eevm::SimpleAccount, GlobalView>;

 public:
  GlobalState(resdb::Storage* storage);
  virtual ~GlobalState() = default;

  virtual void remove(const eevm::Address& addr) override;

  // Get contract by contract address.
  eevm::AccountState get(const eevm::Address& addr) override;

  bool Exists(const eevm::Address& addr);

  // Create an account for the contract, which the balance is 0.
  eevm::AccountState create(const eevm::Address& addr, const uint256_t& balance,
                            const eevm::Code& code) override;

  const eevm::SimpleAccount& GetAccount(const eevm::Address& addr);

  std::string GetBalance(const eevm::Address& account);
  int SetBalance(const eevm::Address& account, const uint256_t& balance);

 protected:
  void Insert(const StateEntry& p);

 private:
  std::map<eevm::Address, StateEntry> accounts;
  resdb::Storage* storage_;
};

}  // namespace contract
}  // namespace resdb
