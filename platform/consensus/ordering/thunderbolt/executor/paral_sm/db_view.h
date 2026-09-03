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

#include "eEVM/storage.h"
#include "platform/consensus/ordering/thunderbolt/executor/paral_sm/concurrency_controller.h"
#include "platform/consensus/ordering/thunderbolt/executor/paral_sm/streaming_e_controller.h"

namespace resdb {
namespace contract {
namespace paral_sm {

class DBView : public eevm::Storage {
 public:
  DBView(ConcurrencyController* controller, int64_t commit_id, int version);
  virtual ~DBView() = default;

  void store(const uint256_t& key, const uint256_t& value) override;
  uint256_t load(const uint256_t& key) override;
  bool remove(const uint256_t& key) override;

  // for 2PL, once it is done, all the commit will be pushed to
  // the controller to judge if it can be committed.
  // During the flesh, all the changes will be removed.
  void Flesh(int64_t commit_id) {}
  // Commit the changes. If there is a conflict, return false.
  // Make sure all other committers have pushed their changes before calling
  // Commit.
  // bool Commit();
  // Remove all the changes.
  // void Abort();

 private:
  StreamingEController* controller_;
  int64_t commit_id_;
  int version_;
  std::map<uint256_t, std::vector<Data>> local_changes_;
};

}  // namespace paral_sm
}  // namespace contract
}  // namespace resdb
