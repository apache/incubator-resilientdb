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
#include "platform/consensus/ordering/thunderbolt/executor/manager/ooo_controller.h"

#include <glog/logging.h>

namespace resdb {
namespace contract {

OOOController::OOOController(DataStorage* storage)
    : ConcurrencyController(storage) {}

OOOController::~OOOController() {}

void OOOController::PushCommit(int64_t commit_id,
                               const ModifyMap& local_changes) {
  for (const auto& it : local_changes) {
    bool done = false;
    for (int i = it.second.size() - 1; i >= 0 && !done; --i) {
      const auto& op = it.second[i];
      switch (op.state) {
        case LOAD:
          break;
        case STORE:
          storage_->Store(it.first, op.data);
          done = true;
          break;
        case REMOVE:
          storage_->Remove(it.first);
          done = true;
          break;
      }
    }
  }
}

}  // namespace contract
}  // namespace resdb
