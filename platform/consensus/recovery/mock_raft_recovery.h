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

#include <gmock/gmock.h>

#include "platform/consensus/recovery/raft_recovery.h"
#include "platform/consensus/checkpoint/mock_checkpoint.h"
#include "chain/storage/mock_storage.h"

namespace resdb {
namespace raft {

class MockRaftRecovery : public RaftRecovery {
 public:
  MockRaftRecovery(const ResDBConfig& config)
      : RaftRecovery(config, mock_checkpoint_.get(), mock_storage_.get(),
                     nullptr) {}

  MOCK_METHOD(void, AddLogEntry, (const Entry* entry), ());
  MOCK_METHOD(void, WriteMetadata,
              (int64_t current_term, int32_t voted_for,
               uint64_t snapshot_last_index, uint64_t snapshot_last_term),
              ());
  MOCK_METHOD(void, AddLogEntry, (std::vector<Entry>& entries_to_add), ());
  MOCK_METHOD(void, TruncateLog, (TruncationRecord truncate_beginning_at), ());

  std::unique_ptr<MockCheckPoint> mock_checkpoint_;
  std::unique_ptr<MockStorage> mock_storage_;
};

}  // namespace raft
}  // namespace resdb
