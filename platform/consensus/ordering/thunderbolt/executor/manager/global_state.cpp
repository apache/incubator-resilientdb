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
#include "platform/consensus/ordering/thunderbolt/executor/manager/global_state.h"

#include <glog/logging.h>

namespace resdb {
namespace contract {

using eevm::AccountState;
using eevm::Address;
using eevm::Code;
using eevm::SimpleAccount;

GlobalState::GlobalState(DataStorage* storage) : storage_(storage) {}

bool GlobalState::Exists(const eevm::Address& addr) {
  return accounts.find(addr) != accounts.cend();
}

void GlobalState::remove(const Address& addr) { accounts.erase(addr); }

AccountState GlobalState::get(const Address& addr) {
  const auto acc = accounts.find(addr);
  if (acc != accounts.cend()) return acc->second;

  return create(addr, 0, {});
}

AccountState GlobalState::create(const Address& addr, const uint256_t& balance,
                                 const Code& code) {
  Insert({SimpleAccount(addr, balance, code), GlobalView(storage_)});

  return get(addr);
}

const eevm::SimpleAccount& GlobalState::GetAccount(const eevm::Address& addr) {
  const auto acc = accounts.find(addr);
  return acc->second.first;
}

void GlobalState::Insert(const StateEntry& p) {
  const auto ib = accounts.insert(std::make_pair(p.first.get_address(), p));

  assert(ib.second);
}
}  // namespace contract
}  // namespace resdb
