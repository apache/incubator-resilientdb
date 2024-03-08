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

#include "absl/status/statusor.h"
#include "interface/rdbc/net_channel.h"
#include "platform/config/resdb_config.h"
#include "platform/proto/replica_info.pb.h"

namespace resdb {

// ResDBTxnAccessor used to obtain the server state of each replica in ResDB.
// The addresses of each replica are provided from the config.
class ResDBTxnAccessor {
 public:
  ResDBTxnAccessor(const ResDBConfig& config);
  virtual ~ResDBTxnAccessor() = default;

  // Obtain ReplicaState of each replica.
  virtual absl::StatusOr<std::vector<std::pair<uint64_t, std::string>>> GetTxn(
      uint64_t min_seq, uint64_t max_seq);

  virtual absl::StatusOr<std::vector<Request>> GetRequestFromReplica(
      uint64_t min_seq, uint64_t max_seq, const ReplicaInfo& replica);
  virtual absl::StatusOr<uint64_t> GetBlockNumbers();

 protected:
  virtual std::unique_ptr<NetChannel> GetNetChannel(const std::string& ip,
                                                    int port);

 private:
  ResDBConfig config_;
  std::vector<ReplicaInfo> replicas_;
  int recv_timeout_ = 1;
};

}  // namespace resdb
