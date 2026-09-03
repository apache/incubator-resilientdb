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
#include <shared_mutex>

#include "platform/consensus/ordering/thunderbolt/executor/manager/concurrency_controller.h"

namespace resdb {
namespace contract {

class TwoPhaseOOOController : public ConcurrencyController {
 public:
  TwoPhaseOOOController(DataStorage* storage);

  virtual void PushCommit(int64_t commit_id, const ModifyMap& local_changes_);

  bool Commit(int64_t commit_id);

  // Before each 2PL, make sure clear the data from the previous round.
  void Clear();

 private:
  bool CheckCommit(int64_t commit_id);

 protected:
  mutable std::shared_mutex mutex_;
  std::vector<ModifyMap> changes_list_;
  std::map<uint256_t, int64_t> first_commit_;
  const int window_size_ = 1000;
};

}  // namespace contract
}  // namespace resdb
