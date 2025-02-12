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

#include "executor/contract/manager/address_manager.h"

#include <glog/logging.h>

#include "eEVM/util.h"

namespace resdb {
namespace contract {

Address AddressManager::CreateRandomAddress() {
  std::vector<uint8_t> raw(20);
  std::generate(raw.begin(), raw.end(), []() { return rand(); });
  Address address = eevm::from_big_endian(raw.data(), raw.size());
  users_.insert(address);
  return address;
}

void AddressManager::AddExternalAddress(const Address& address) {
  users_.insert(address);  // New method implementation
}

bool AddressManager::Exist(const Address& address) {
  return users_.find(address) != users_.end();
}

Address AddressManager::CreateContractAddress(const Address& owner) {
  return eevm::generate_address(owner, 0u);
}

std::string AddressManager::AddressToHex(const Address& address) {
  return eevm::to_hex_string(address);
}

Address AddressManager::HexToAddress(const std::string& address) {
  return eevm::to_uint256(address);
}

}  // namespace contract
}  // namespace resdb
