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

#include "eEVM/util.h"
#include "glog/logging.h"
#include "gtest/gtest.h"

namespace resdb {
namespace contract {
namespace {

TEST(AddressManagerTest, CreateAddress) {
  Address address = AddressManager().CreateRandomAddress();
  std::array<uint8_t, 32> raw;
  eevm::to_big_endian(address, raw.data());
  Address m = eevm::from_big_endian(raw.data() + 12, raw.size() - 12);
  EXPECT_EQ(m, address);
}

TEST(AddressManagerTest, CreateContractAddress) {
  Address address = AddressManager().CreateRandomAddress();

  Address contract_address = AddressManager::CreateContractAddress(address);

  std::array<uint8_t, 32> raw;
  eevm::to_big_endian(contract_address, raw.data());
  Address m = eevm::from_big_endian(raw.data() + 12, raw.size() - 12);
  EXPECT_EQ(m, contract_address);
}

}  // namespace
}  // namespace contract
}  // namespace resdb
