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

#include "client/resdb_client.h"
#include "common/queue/batch_queue.h"
#include "common/queue/lock_free_queue.h"
#include "proto/replica_info.pb.h"
#include "proto/resdb.pb.h"
#include "server/async_replica_client.h"
#include "statistic/stats.h"

namespace resdb {

// ResDBReplicaClient is used for replicas to broadcast messages.
class ResDBReplicaClient {
 public:
  ResDBReplicaClient(const std::vector<ReplicaInfo>& replicas,
                     SignatureVerifier* verifier = nullptr,
                     bool is_use_long_conn = false, int epoll_num = 1,
                     int tcp_batch = 100);
  virtual ~ResDBReplicaClient();

  // HeartBeat message is used to broadcast public keys.
  // It doesn't need the signature.
  virtual int SendHeartBeat(const Request& hb_info);

  virtual int SendMessage(const google::protobuf::Message& message);
  virtual int SendMessage(const google::protobuf::Message& message,
                          const ReplicaInfo& replica_info);

  virtual void BroadCast(const google::protobuf::Message& message);
  virtual void SendMessage(const google::protobuf::Message& message,
                           int64_t node_id);
  virtual int SendBatchMessage(
      const std::vector<std::unique_ptr<Request>>& messages,
      const ReplicaInfo& replica_info);

  void UpdateClientReplicas(const std::vector<ReplicaInfo>& replicas);
  std::vector<ReplicaInfo> GetClientReplicas();

 protected:
  virtual std::unique_ptr<ResDBClient> GetClient(const std::string& ip,
                                                 int port);
  virtual AsyncReplicaClient* GetClientFromPool(const std::string& ip,
                                                int port);

  void StartBroadcastInBackGround();
  int SendMessageInternal(const google::protobuf::Message& message,
                          const std::vector<ReplicaInfo>& replicas);
  int SendMessageFromPool(const google::protobuf::Message& message,
                          const std::vector<ReplicaInfo>& replicas);

  bool IsRunning() const;
  bool IsInPool(const ReplicaInfo& replica_info);

 private:
  std::vector<ReplicaInfo> replicas_;
  SignatureVerifier* verifier_;
  std::map<std::pair<std::string, int>, std::unique_ptr<AsyncReplicaClient>>
      client_pools_;
  std::thread broadcast_thread_;
  std::atomic<bool> is_running_;
  struct QueueItem {
    std::string data;
    std::vector<ReplicaInfo> dest_replicas;
  };
  BatchQueue<std::unique_ptr<QueueItem>> batch_queue_;
  bool is_use_long_conn_ = false;

  Stats* global_stats_;
  boost::asio::io_service io_service_;
  std::unique_ptr<boost::asio::io_service::work> worker_;
  std::vector<std::thread> worker_threads_;
  std::vector<ReplicaInfo> clients_;
  std::mutex mutex_;
};

}  // namespace resdb
