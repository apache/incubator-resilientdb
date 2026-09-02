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

#include "platform/consensus/ordering/thunderbolt/executor/x_manager/data_storage.h"
#include "platform/consensus/ordering/thunderbolt/executor/common/rws_types.h"

namespace resdb {
namespace contract {
namespace x_manager {

class ConcurrencyController {
 public:
  ConcurrencyController(DataStorage* storage);

  using ModifyMap = ::resdb::contract::ModifyMap;

  virtual void PushCommit(int64_t commit_id,
                          const ModifyMap& local_changes_) = 0;

  const DataStorage* GetStorage() const;

 protected:
  DataStorage* storage_;
};

}  // namespace x_manager
}  // namespace contract
}  // namespace resdb
