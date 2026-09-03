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
#include "platform/consensus/ordering/thunderbolt/executor/manager/concurrency_controller.h"
#include "platform/consensus/ordering/thunderbolt/executor/manager/d_storage.h"

namespace resdb {
namespace contract {
namespace streaming {

class StreamingDQController : public ConcurrencyController {
 public:
  StreamingDQController(DataStorage* storage, int window_size);
  ~StreamingDQController();

  virtual void PushCommit(int64_t commit_id, const ModifyMap& local_changes_);
  bool Commit(int64_t commit_id);

  std::vector<int64_t>& GetRedo();
  std::vector<int64_t>& GetDone();

  typedef std::map<uint256_t, std::deque<int64_t> > CommitList;

  void Clear(int64_t commit_id);
  void Clear();

 private:
  bool PreCommit(int64_t commit_id);
  bool PreCommitInternal(int64_t commit_id);
  void MergeChangeList(int64_t commit_id);

  bool PostCommit(int64_t commit_id);
  void RedoConflict(int64_t commit_id);

  bool CheckCommit(int64_t commit_id);
  bool CheckPreCommit(int64_t commit_id);

  const ModifyMap* GetChangeList(int64_t commit_id);
  void Remove(int64_t commit_id);

  void CommitDone(int64_t commit_id);
  void RedoCommit(int64_t commit_id, int flag);

  void RollBackData(const uint256_t& address, const Data& data);

  bool IsRead(const uint256_t& address, int64_t commit_id);

 private:
  int window_size_ = 2000;

  std::vector<ModifyMap> changes_list_, rechanges_list_;

  CommitList commit_list_[4196];
  int64_t last_commit_id_, current_commit_id_;

  CommitList pre_commit_list_[4196];
  int64_t last_pre_commit_id_;

  std::atomic<int> is_redo_[4196];
  std::vector<int64_t> redo_;
  bool wait_[4196], committed_[4196], finish_[4196];

  // std::atomic<bool> is_done_[1024];
  std::vector<int64_t> done_;

  DataStorage* storage_;
};

}  // namespace streaming
}  // namespace contract
}  // namespace resdb
