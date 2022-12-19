/*
 * Copyright (c) 2019-2022 ExpoLab, UC Davis
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#pragma once

#include <thread>

#include "common/queue/blocking_queue.h"
#include "config/resdb_config.h"
#include "proto/replica_info.pb.h"
#include "proto/resdb.pb.h"
#include "server/resdb_replica_client.h"
#include "server/resdb_service.h"
#include "statistic/stats.h"

namespace resdb {

// ConsensusService is an consus algorithm implimentation of ConsensusService.
// It receives the messages from ResDBServer and running a consus algorithm
// like PBFT to commit.
// It also handles the public key information exchanged from others.
class ConsensusService : public ResDBService {
 public:
  ConsensusService(const ResDBConfig& config);
  virtual ~ConsensusService();

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

  virtual std::unique_ptr<ResDBReplicaClient> GetReplicaClient(
      const std::vector<ReplicaInfo>& replicas, bool is_use_long_conn = false);

  virtual std::vector<ReplicaInfo> GetReplicas() = 0;
  std::vector<ReplicaInfo> GetAllReplicas();
  virtual std::vector<ReplicaInfo> GetClientReplicas();
  virtual void AddNewReplica(const ReplicaInfo& info);
  virtual uint32_t GetPrimary();
  virtual uint32_t GetVersion();
  virtual void SetPrimary(uint32_t primary, uint64_t version);
  void AddNewClient(const ReplicaInfo& info);

  ResDBReplicaClient* GetBroadCastClient();
  // Update broad cast client to reflush the replica list.
  void UpdateBroadCastClient();

  SignatureVerifier* GetSignatureVerifier();

 private:
  void HeartBeat();
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
  std::unique_ptr<ResDBReplicaClient> bc_client_;
  std::vector<ReplicaInfo> clients_;
  Stats* global_stats_;
};

}  // namespace resdb
