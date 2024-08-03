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

#include "platform/consensus/execution/system_info.h"

#include <glog/logging.h>

namespace resdb {

SystemInfo::SystemInfo() : primary_id_(1), view_(1) {}

SystemInfo::SystemInfo(const ResDBConfig& config)
    : primary_id_(config.GetReplicaInfos()[0].id()), view_(1) {
  SetReplicas(config.GetReplicaInfos());
  LOG(ERROR) << "get primary id:" << primary_id_;
}

uint32_t SystemInfo::GetPrimaryId() const { return primary_id_; }

void SystemInfo::SetPrimary(uint32_t id) { primary_id_ = id; }

uint64_t SystemInfo::GetCurrentView() const { return view_; }

void SystemInfo::SetCurrentView(uint64_t view_id) { view_ = view_id; }

std::vector<ReplicaInfo> SystemInfo::GetReplicas() const { return replicas_; }

void SystemInfo::SetReplicas(const std::vector<ReplicaInfo>& replicas) {
  replicas_ = replicas;
}

void SystemInfo::AddReplica(const ReplicaInfo& replica) {
  if (replica.id() == 0 || replica.ip().empty() || replica.port() == 0) {
    return;
  }
  for (const auto& cur_replica : replicas_) {
    if (cur_replica.id() == replica.id()) {
      LOG(ERROR) << " replica exist:" << replica.id();
      return;
    }
  }
  LOG(ERROR) << "add new replica:" << replica.DebugString();
  replicas_.push_back(replica);
}

void SystemInfo::ProcessRequest(const SystemInfoRequest& request) {
  switch (request.type()) {
    case SystemInfoRequest::ADD_REPLICA: {
      NewReplicaRequest info;
      if (info.ParseFromString(request.request())) {
        AddReplica(info.replica_info());
      }
    } break;
    default:
      break;
  }
}

}  // namespace resdb
