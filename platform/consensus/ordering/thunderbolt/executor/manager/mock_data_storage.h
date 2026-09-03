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
#pragma once

#include "gmock/gmock.h"
#include "platform/consensus/ordering/thunderbolt/executor/manager/data_storage.h"

namespace resdb {
namespace contract {

class MockStorage : public DataStorage {
 public:
  typedef std::pair<uint256_t, int64_t> LoadType;

  MOCK_METHOD(int64_t, Store,
              (const uint256_t& key, const uint256_t& value, bool), (override));
  MOCK_METHOD(bool, Remove, (const uint256_t&, bool), (override));
  MOCK_METHOD(bool, Exist, (const uint256_t&, bool), (const, override));
  MOCK_METHOD(int64_t, GetVersion, (const uint256_t&, bool), (const, override));
  MOCK_METHOD(LoadType, Load, (const uint256_t&, bool), (const, override));
};

}  // namespace contract
}  // namespace resdb
