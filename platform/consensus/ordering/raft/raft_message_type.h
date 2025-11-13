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

namespace resdb {
namespace raft {

// These values occupy Request::user_type within TYPE_CUSTOM_CONSENSUS.
enum : int {
  RAFT_APPEND_ENTRIES_REQUEST = 1,
  RAFT_APPEND_ENTRIES_RESPONSE = 2,
  RAFT_REQUEST_VOTE_REQUEST = 3,
  RAFT_REQUEST_VOTE_RESPONSE = 4,
  RAFT_INSTALL_SNAPSHOT_REQUEST = 5,
  RAFT_INSTALL_SNAPSHOT_RESPONSE = 6,
  RAFT_MESSAGE_MAX
};

inline bool IsRaftUserType(int user_type) {
  return user_type >= RAFT_APPEND_ENTRIES_REQUEST && user_type < RAFT_MESSAGE_MAX;
}

}  // namespace raft
}  // namespace resdb

