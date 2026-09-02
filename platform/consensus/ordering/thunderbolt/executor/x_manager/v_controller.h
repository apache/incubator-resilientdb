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

#include <deque>
#include <set>
#include <shared_mutex>
#include <thread>

#include "platform/common/queue/lock_free_queue.h"
#include "platform/consensus/ordering/thunderbolt/executor/x_manager/concurrency_controller.h"

namespace resdb {
namespace contract {
namespace x_manager {

class VController : public ConcurrencyController {
 public:
  VController(DataStorage* storage);
  ~VController();

  virtual void PushCommit(int64_t commit_id, const ModifyMap& local_changes_);
  bool Commit(int64_t commit_id);
  const ModifyMap* GetChangeList(int64_t commit_id) const;

 private:
  bool CheckCommit(int64_t commit_id);

 private:
  const int window_size_ = 2000;
  std::vector<ModifyMap> changes_list_;
};

}  // namespace x_manager
}  // namespace contract
}  // namespace resdb
