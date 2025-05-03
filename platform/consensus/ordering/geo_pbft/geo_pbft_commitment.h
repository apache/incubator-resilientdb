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

#include "platform/config/resdb_config.h"
#include "platform/consensus/execution/geo_global_executor.h"
#include "platform/consensus/execution/system_info.h"
#include "platform/consensus/ordering/geo_pbft/hash_set.h"
#include "platform/networkstrate/replica_communicator.h"
#include "platform/networkstrate/server_comm.h"
#include "platform/proto/resdb.pb.h"

namespace resdb {

class GeoPBFTCommitment {
 public:
  GeoPBFTCommitment(std::unique_ptr<GeoGlobalExecutor> global_executor,
                    const ResDBConfig& config,
                    std::unique_ptr<SystemInfo> system_info_,
                    ReplicaCommunicator* replica_communicator_,
                    SignatureVerifier* verifier);

  ~GeoPBFTCommitment();

  int GeoProcessCcm(std::unique_ptr<Context> context,
                    std::unique_ptr<Request> request);

 private:
  bool VerifyCerts(const BatchUserRequest& request,
                   const std::string& raw_data);

  bool AddNewReq(uint64_t seq, uint32_t sender_region);
  void UpdateSeq(uint64_t seq);

  int PostProcessExecutedMsg();

 private:
  std::unique_ptr<GeoGlobalExecutor> global_executor_;
  std::atomic<bool> stop_;
  std::set<uint32_t> checklist_[1 << 20];
  ResDBConfig config_;
  std::unique_ptr<SystemInfo> system_info_ = nullptr;
  ReplicaCommunicator* replica_communicator_;
  SignatureVerifier* verifier_;
  Stats* global_stats_;
  std::mutex mutex_;
  std::thread executed_thread_;
  uint64_t min_seq_ = 0;
};

}  // namespace resdb
