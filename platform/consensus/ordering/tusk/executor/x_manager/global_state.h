// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "service/contract/executor/x_manager/global_view.h"
#include "service/contract/executor/x_manager/evm_state.h"

#include "eEVM/simple/simpleaccount.h"

namespace resdb {
namespace contract {
namespace x_manager {

  class GlobalState : public EVMState{
  public:
    using StateEntry = std::pair<eevm::SimpleAccount, GlobalView>;

  public:
    GlobalState(DataStorage* storage);
    virtual ~GlobalState() = default;

    virtual void remove(const eevm::Address& addr) override;

    // Get contract by contract address.
    eevm::AccountState get(const eevm::Address& addr) override;

    bool Exists(const eevm::Address& addr);

    // Create an account for the contract, which the balance is 0.
    eevm::AccountState create(
      const eevm::Address& addr, const uint256_t& balance, const eevm::Code& code) override;

    const eevm::SimpleAccount& GetAccount(const eevm::Address& addr) ;

  protected:
    void Insert(const StateEntry& p);

  private:
    std::map<eevm::Address, StateEntry> accounts;
    DataStorage* storage_;
  };

}
}
} // namespace eevm
