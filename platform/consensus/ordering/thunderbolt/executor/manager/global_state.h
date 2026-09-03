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
#include "platform/consensus/ordering/thunderbolt/executor/manager/evm_state.h"
#include "platform/consensus/ordering/thunderbolt/executor/manager/global_view.h"

namespace resdb {
namespace contract {

class GlobalState : public EVMState {
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
  eevm::AccountState create(const eevm::Address& addr, const uint256_t& balance,
                            const eevm::Code& code) override;

  const eevm::SimpleAccount& GetAccount(const eevm::Address& addr);

 protected:
  void Insert(const StateEntry& p);

 private:
  std::map<eevm::Address, StateEntry> accounts;
  DataStorage* storage_;
};

}  // namespace contract
}  // namespace resdb
