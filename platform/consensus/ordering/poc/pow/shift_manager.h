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

#include <condition_variable>

#include "platform/config/resdb_poc_config.h"
#include "platform/consensus/ordering/poc/proto/pow.pb.h"

namespace resdb {

class ShiftManager {
 public:
  ShiftManager(const ResDBPoCConfig& config);
  virtual ~ShiftManager() = default;

  void AddSliceInfo(const SliceInfo& slice_info);
  virtual bool Check(const SliceInfo& slice_info, int timeout_ms = 10000);

 private:
  ResDBPoCConfig config_;
  std::map<std::pair<uint64_t, uint64_t>, std::set<uint32_t>> data_;
  std::mutex mutex_;
  std::condition_variable cv_;
};

}  // namespace resdb
