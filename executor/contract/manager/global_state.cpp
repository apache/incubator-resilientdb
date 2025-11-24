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


#include "executor/contract/manager/global_state.h"

#include <glog/logging.h>

namespace resdb {
namespace contract {

using eevm::AccountState;
using eevm::Address;
using eevm::Code;
using eevm::SimpleAccount;


uint256_t AccountToAddress(const eevm::Address& account) {
    std::vector<uint8_t> code;
    code.resize(64);
    std::fill(code.begin(), code.end(), 0);
    eevm::to_big_endian(account, code.data());

    uint8_t h[32];
    eevm::keccak_256(code.data(), static_cast<unsigned int>(64), h);
    return eevm::from_big_endian(h, sizeof(h));
}

GlobalState::GlobalState(resdb::Storage* storage) : storage_(storage) {}

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

std::string GlobalState::GetBalance(const eevm::Address& account) {
  std::string key = "contract_balance_" + eevm::to_hex_string(AccountToAddress(account));
  return storage_->GetValue(key);
}

int GlobalState::SetBalance(const eevm::Address& account, const uint256_t& balance) {
  std::string key = "contract_balance_" + eevm::to_hex_string(AccountToAddress(account));
  return storage_->SetValue(key, eevm::to_hex_string(balance));
}

}  // namespace contract
}  // namespace resdb
