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

namespace resdb {
namespace contract {

class XController : public ConcurrencyController {
 public:
  struct CallBack {
    std::function<void(int64_t)> redo_callback = nullptr;
    std::function<void(int64_t)> committed_callback = nullptr;
  };

  XController(DataStorage* storage);
  ~XController();

  void SetCallback(CallBack call_back);

  virtual void PushCommit(int64_t commit_id, const ModifyMap& local_changes_);
  bool Commit(int64_t commit_id);
  std::vector<int64_t>& GetRedo();

  void Clear();
  const ModifyMap* GetChangeList(int64_t commit_id);

  void UseOCC();

 private:
  bool CommitInternal(int64_t commit_id);
  bool CheckCommit(int64_t commit_id);
  bool CheckFirstCommit(const uint256_t& address, int64_t commit_id);

  // int64_t Remove(const uint256_t& address, int64_t commit_id, uint64_t v);
  void Remove(int64_t commit_id);

  std::function<void(int64_t)> GetCommitCallBack(int64_t commit_id);
  std::function<void(int64_t)> GetRedoCallBack(int64_t commit_id);

  void CommitDone(int64_t commit_id);
  void RedoCommit(int64_t commit_id);

  void CheckRedo();
  void AddRedo(int64_t commit_id);

  int GetHashKey(const uint256_t& address);

  bool IsRead(const uint256_t& address, int64_t commit_id);

 private:
  const int window_size_ = 4096;
  mutable std::shared_mutex mutex_, mutexs_[4096], cmutex_;

  std::map<uint256_t, int64_t> first_commit_;
  std::atomic<int64_t> last_commit_id_;

  std::vector<ModifyMap> changes_list_, pending_list_;
  std::map<uint256_t, std::deque<int64_t> > commit_list_;
  // std::map<uint256_t, std::deque<int64_t> > commit_list_[2048];
  // std::map<uint256_t, std::set<int64_t> > commit_list_[1024];
  std::map<uint256_t, uint64_t> m_list_;

  std::vector<bool> is_redo_;
  CallBack call_back_;

  std::vector<int64_t> redo_;
  std::set<int64_t> redo_list_;
  std::map<uint256_t, int> ref_;
  bool use_occ_ = false;
};

}  // namespace contract
}  // namespace resdb
