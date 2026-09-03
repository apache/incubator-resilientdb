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
#include "platform/consensus/ordering/thunderbolt/executor/x_manager/db_view.h"

#include <glog/logging.h>

#include "eEVM/util.h"

namespace resdb {
namespace contract {
namespace x_manager {

DBView::DBView(ConcurrencyController* controller, int64_t commit_id,
               int version)
    : controller_(static_cast<StreamingEController*>(controller)),
      commit_id_(commit_id),
      version_(version) {}

void DBView::store(const uint256_t& key, const uint256_t& value) {
  // LOG(ERROR)<<"store:"<<key<<" value:"<<value;
  controller_->Store(commit_id_, key, value, version_);
}

uint256_t DBView::load(const uint256_t& key) {
  // LOG(ERROR)<<"laod:"<<key;
  return controller_->Load(commit_id_, key, version_);
}

bool DBView::remove(const uint256_t& key) {
  // LOG(ERROR)<<"remove key:"<<key;
  return controller_->Remove(commit_id_, key, version_);
}

/*
void DBView::Flesh(int64_t commit_id) {
  commit_id_ = commit_id;
  //LOG(ERROR)<<"commit push:"<<commit_id;
  controller_->PushCommit(commit_id, local_changes_);
  local_changes_.clear();
}
*/

}  // namespace x_manager
}  // namespace contract
}  // namespace resdb
