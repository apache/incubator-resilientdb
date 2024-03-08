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

// ResDBStateAccessor used to obtain the server state of each replica in ResDB.
// The addresses of each replica are provided from the config.
class ResDBStateAccessor {
 public:
  ResDBStateAccessor(const ResDBConfig& config);
  virtual ~ResDBStateAccessor() = default;

  // Obtain ReplicaState of each replica.
  absl::StatusOr<ReplicaState> GetReplicaState();

 protected:
  virtual std::unique_ptr<NetChannel> GetNetChannel(const std::string& ip,
                                                    int port);

 private:
  ResDBConfig config_;
};

}  // namespace resdb
