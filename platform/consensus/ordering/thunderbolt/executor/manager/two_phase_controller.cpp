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
#include "platform/consensus/ordering/thunderbolt/executor/manager/two_phase_controller.h"

#include <glog/logging.h>

namespace resdb {
namespace contract {

TwoPhaseController::TwoPhaseController(DataStorage* storage)
    : ConcurrencyController(storage) {}

void TwoPhaseController::Clear() {
  std::unique_lock lock(mutex_);
  changes_list_.clear();
  first_commit_.clear();

  changes_list_.resize(window_size_);
  for (int i = 0; i < window_size_; ++i) {
    changes_list_[i].clear();
  }
}

void TwoPhaseController::PushCommit(int64_t commit_id,
                                    const ModifyMap& local_changes) {
  if (!changes_list_[commit_id].empty()) {
    LOG(ERROR) << "push record:" << commit_id << " exists";
    return;
  }

  changes_list_[commit_id] = local_changes;

  std::unique_lock lock(mutex_);
  for (auto it : local_changes) {
    auto first_it = first_commit_.find(it.first);
    bool is_all_read = true;
    for (auto& op : it.second) {
      if (op.state != LOAD) {
        is_all_read = false;
        break;
      }
    }
    if (is_all_read) {
      continue;
    }
    if (first_it == first_commit_.end() || first_it->second > commit_id) {
      first_commit_[it.first] = commit_id;
    }
  }
}

bool TwoPhaseController::CheckCommit(int64_t commit_id) {
  const auto& change_set = changes_list_[commit_id];
  if (change_set.empty()) {
    LOG(ERROR) << " no commit id record found:" << commit_id;
    return false;
  }

  std::shared_lock lock(mutex_);
  for (const auto& it : change_set) {
    auto first_it = first_commit_.find(it.first);
    if (first_it == first_commit_.end()) {
      bool is_all_read = true;
      for (auto& op : it.second) {
        if (op.state != LOAD) {
          is_all_read = false;
          break;
        }
      }
      if (is_all_read) {
        continue;
      }
      return false;
    }
    if (first_it->second < commit_id) {
      return false;
    }
  }
  return true;
}

bool TwoPhaseController::Commit(int64_t commit_id) {
  if (!CheckCommit(commit_id)) {
    return false;
  }

  const auto& change_set = changes_list_[commit_id];
  if (change_set.empty()) {
    LOG(ERROR) << " no commit id record found:" << commit_id;
    return false;
  }
  for (const auto& it : change_set) {
    bool done = false;
    for (int i = it.second.size() - 1; i >= 0 && !done; --i) {
      const auto& op = it.second[i];
      switch (op.state) {
        case LOAD:
          break;
        case STORE:
          // LOG(ERROR)<<"commit:"<<it.first<<" data:"<<op.data<<" commit
          // id:"<<commit_id;
          storage_->Store(it.first, op.data);
          done = true;
          break;
        case REMOVE:
          // LOG(ERROR)<<"remove:"<<it.first<<" data:";
          storage_->Remove(it.first);
          done = true;
          break;
      }
    }
  }
  return true;
}

}  // namespace contract
}  // namespace resdb
