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
#include "platform/consensus/ordering/thunderbolt/executor/paral_sm/streaming_e_controller.h"

namespace resdb {
namespace contract {
namespace paral_sm {

class MockEController : public StreamingEController {
 public:
  MockEController(DataStorage* storage, int window)
      : StreamingEController(storage, window) {}

  MOCK_METHOD(void, Store,
              (const int64_t, const uint256_t& key, const uint256_t& value,
               int),
              (override));
  MOCK_METHOD(uint256_t, Load, (const int64_t, const uint256_t&, int),
              (override));
};

}  // namespace paral_sm
}  // namespace contract
}  // namespace resdb
