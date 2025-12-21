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
#include <unordered_map>

#include "platform/proto/resdb.pb.h"

namespace resdb {

class ChainState {
 public:
  ChainState();
  Request* Get(uint64_t seq);
  void Put(std::unique_ptr<Request> request);
  uint64_t GetMaxSeq();

 private:
  std::mutex mutex_;
  std::unordered_map<uint64_t, std::unique_ptr<Request> > data_;
  std::atomic<uint64_t> max_seq_;
};

}  // namespace resdb
