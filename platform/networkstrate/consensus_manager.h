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

#include <thread>

#include "platform/common/queue/blocking_queue.h"
#include "platform/config/resdb_config.h"
#include "platform/networkstrate/replica_communicator.h"
#include "platform/networkstrate/service_interface.h"
#include "platform/proto/replica_info.pb.h"
#include "platform/proto/resdb.pb.h"
#include "platform/statistic/stats.h"

namespace resdb {

// ConsensusManager is an consensus algorithm implimentation.
// It receives the messages from ResDBServer and running a consensus algorithm
// like PBFT to commit.
class ConsensusManager : public ServiceInterface {
 public:
  ConsensusManager(const ResDBConfig& config);
  virtual ~ConsensusManager();

  // Process a request receied from ResDBServer.
  // context contains the client socket and request_info contains the data
  // received from the network.
  virtual int Process(std::unique_ptr<Context> context,
                      std::unique_ptr<DataInfo> request_info);

  bool IsReady() const;
  void Stop();

  // Should be called by the instance or test.
  void Start();

 protected:
  // BroadCast will generate signatures whiling sending data to other replicas.
  virtual void BroadCast(const Request& request);
  virtual void SendMessage(const google::protobuf::Message& message,
                           int64_t node_id);

  virtual int Dispatch(std::unique_ptr<Context> context,
                       std::unique_ptr<Request> request);
  virtual int ConsensusCommit(std::unique_ptr<Context> context,
                              std::unique_ptr<Request> request);

  // =============== default function ======================
  int ProcessClientCert(std::unique_ptr<Context> context,
                        std::unique_ptr<Request> request);
  int ProcessHeartBeat(std::unique_ptr<Context> context,
                       std::unique_ptr<Request> request);
  // =======================================================

  virtual std::unique_ptr<ReplicaCommunicator> GetReplicaClient(
      const std::vector<ReplicaInfo>& replicas, bool is_use_long_conn = false);

  virtual std::vector<ReplicaInfo> GetReplicas() = 0;
  std::vector<ReplicaInfo> GetAllReplicas();
  virtual std::vector<ReplicaInfo> GetClientReplicas();
  virtual void AddNewReplica(const ReplicaInfo& info);
  virtual uint32_t GetPrimary();
  virtual uint32_t GetVersion();
  virtual void SetPrimary(uint32_t primary, uint64_t version);
  void AddNewClient(const ReplicaInfo& info);

  ReplicaCommunicator* GetBroadCastClient();
  // Update broad cast client to reflush the replica list.
  void UpdateBroadCastClient();

  SignatureVerifier* GetSignatureVerifier();

 private:
  void HeartBeat();
  void SendHeartBeat();
  void BroadCastThread();

 protected:
  ResDBConfig config_;
  std::unique_ptr<SignatureVerifier> verifier_;
  struct QueueItem {
    std::unique_ptr<Request> request;
    std::vector<ReplicaInfo> replicas;
  };

 private:
  std::thread heartbeat_thread_;
  std::atomic<bool> is_ready_ = false;
  std::unique_ptr<ReplicaCommunicator> bc_client_;
  std::vector<ReplicaInfo> clients_;
  Stats* global_stats_;
  uint64_t version_;
  std::map<int, uint64_t> hb_;
  std::mutex hb_mutex_;
};

}  // namespace resdb
