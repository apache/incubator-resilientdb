/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "eEVM/simple/simpleaccount.h"
#include "platform/consensus/ordering/thunderbolt/executor/paral_sm/concurrency_controller.h"
#include "platform/consensus/ordering/thunderbolt/executor/paral_sm/evm_state.h"
#include "platform/consensus/ordering/thunderbolt/executor/paral_sm/local_view.h"

namespace resdb {
namespace contract {

class LocalState : public EVMState {
 public:
  using StateEntry = std::pair<eevm::SimpleAccount, LocalView>;

 public:
  LocalState(ConcurrencyController* controller);
  virtual ~LocalState() = default;

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
  void Set(const eevm::SimpleAccount& acc, int64_t commit_id);

 protected:
  void Insert(const StateEntry& p);

 private:
  std::map<eevm::Address, StateEntry> accounts;
  ConcurrencyController* controller_;
};

}  // namespace contract
}  // namespace resdb
