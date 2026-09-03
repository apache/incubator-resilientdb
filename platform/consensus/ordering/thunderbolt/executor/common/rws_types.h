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

#include <map>
#include <vector>

#include "platform/consensus/ordering/thunderbolt/executor/common/utils.h"

namespace resdb {
namespace contract {

enum State {
  LOAD = 0,
  STORE = 1,
  REMOVE = 2,
};

struct Data {
  State state;
  uint256_t data;
  int64_t version;
  uint256_t old_data;
  int commit_version;
  bool has_read = false;
  bool invalid = false;

  Data() {}
  Data(const State& state) : state(state) {}
  Data(const State& state, const uint256_t& data, int64_t version = 0)
      : state(state), data(data), version(version) {}
  Data(const State& state, const uint256_t& data, int64_t version,
       const uint256_t& old_data)
      : state(state), data(data), version(version), old_data(old_data) {}

  bool operator!=(const Data& other) const {
    return state != other.state || data != other.data ||
           version != other.version;
  }
};

using ModifyMap = std::map<uint256_t, std::vector<Data>>;

}  // namespace contract
}  // namespace resdb
