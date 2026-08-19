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

#include "platform/consensus/checkpoint/checkpoint.h"

namespace resdb {

class RaftCheckPoint : public CheckPoint {
 public:
  RaftCheckPoint() = default;
  virtual ~RaftCheckPoint() = default;

  virtual uint64_t GetStableCheckpoint() { return current_stable_seq_.load(); }
  virtual void SetStableCheckpoint(uint64_t current_stable_seq) {
    current_stable_seq_.store(current_stable_seq);
  }

 private:
  std::atomic<uint64_t> current_stable_seq_ = 0;
};

}  // namespace resdb
