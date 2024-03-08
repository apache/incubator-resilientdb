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

#include "chain/state/chain_state.h"

#include <glog/logging.h>

namespace resdb {

ChainState::ChainState() : max_seq_(0) {}

Request* ChainState::Get(uint64_t seq) {
  std::unique_lock<std::mutex> lk(mutex_);
  if (data_.find(seq) == data_.end()) {
    return nullptr;
  }
  return data_[seq].get();
}

void ChainState::Put(std::unique_ptr<Request> request) {
  std::unique_lock<std::mutex> lk(mutex_);
  max_seq_ = request->seq();
  data_[max_seq_] = std::move(request);
}

uint64_t ChainState::GetMaxSeq() { return max_seq_; }

}  // namespace resdb
