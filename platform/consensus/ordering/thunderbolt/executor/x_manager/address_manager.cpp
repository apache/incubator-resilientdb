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
#include "platform/consensus/ordering/thunderbolt/executor/x_manager/address_manager.h"

#include <glog/logging.h>

#include "eEVM/util.h"

namespace resdb {
namespace contract {
namespace x_manager {

Address AddressManager::CreateRandomAddress() {
  std::vector<uint8_t> raw(20);
  std::generate(raw.begin(), raw.end(), []() { return rand(); });
  Address address = eevm::from_big_endian(raw.data(), raw.size());
  users_.insert(address);
  return address;
}

bool AddressManager::CreateAddress(const Address& address) {
  if (users_.find(address) != users_.end()) {
    return false;
  }
  users_.insert(address);
  return true;
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

uint256_t AddressManager::AddressToSHAKey(const Address& address) {
  std::vector<uint8_t> code;
  code.resize(64u);
  eevm::to_big_endian(address, code.data());
  // code[63]=1;
  // LOG(ERROR)<<"code size:"<<code.size();

  uint8_t h[32];
  eevm::keccak_256(code.data(), static_cast<unsigned int>(code.size()), h);
  // LOG(ERROR)<<"get h:"<<eevm::from_big_endian(h, sizeof(h));
  return eevm::from_big_endian(h, sizeof(h));
}

}  // namespace x_manager
}  // namespace contract
}  // namespace resdb
