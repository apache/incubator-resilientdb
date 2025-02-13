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

#include "platform/networkstrate/replica_communicator.h"

#include <glog/logging.h>

#include <thread>

#include "platform/proto/broadcast.pb.h"

namespace resdb {

ReplicaCommunicator::ReplicaCommunicator(
    const std::vector<ReplicaInfo>& replicas, SignatureVerifier* verifier,
    bool is_use_long_conn, int epoll_num, int tcp_batch)
    : replicas_(replicas),
      verifier_(verifier),
      is_running_(false),
      batch_queue_("bc_batch", tcp_batch),
      is_use_long_conn_(is_use_long_conn) {
  global_stats_ = Stats::GetGlobalStats();
  if (is_use_long_conn_) {
    worker_ = std::make_unique<boost::asio::io_service::work>(io_service_);
    for (int i = 0; i < epoll_num; ++i) {
      worker_threads_.push_back(std::thread([&]() { io_service_.run(); }));
    }
  }

  /*
  for (const ReplicaInfo& info : replicas) {
    std::string ip = info.ip();
    int port = info.port();
    auto client = std::make_unique<AsyncReplicaClient>(
        &io_service_, ip, port + (is_use_long_conn_ ? 10000 : 0), true);
    client_pools_[std::make_pair(ip, port)] = std::move(client);
  }
  */

  StartBroadcastInBackGround();
}

ReplicaCommunicator::~ReplicaCommunicator() {
  is_running_ = false;
  if (broadcast_thread_.joinable()) {
    broadcast_thread_.join();
  }
  if (is_use_long_conn_) {
    for (auto& cli : client_pools_) {
      cli.second.reset();
    }
    worker_.reset();
    worker_ = nullptr;
    io_service_.stop();
    for (auto& worker_th : worker_threads_) {
      if (worker_th.joinable()) {
        worker_th.join();
      }
    }
  }
}

bool ReplicaCommunicator::IsInPool(const ReplicaInfo& replica_info) {
  for (auto& replica : replicas_) {
    if (replica_info.ip() == replica.ip() &&
        replica_info.port() == replica.port()) {
      return true;
    }
  }
  return false;
}

bool ReplicaCommunicator::IsRunning() const { return is_running_; }

void ReplicaCommunicator::UpdateClientReplicas(
    const std::vector<ReplicaInfo>& replicas) {
  clients_ = replicas;
}

std::vector<ReplicaInfo> ReplicaCommunicator::GetClientReplicas() {
  return clients_;
}

int ReplicaCommunicator::SendHeartBeat(const Request& hb_info) {
  int ret = 0;
  for (const auto& replica : replicas_) {
    NetChannel client(replica.ip(), replica.port());
    if (client.SendRawMessage(hb_info) == 0) {
      ret++;
    }
  }
  return ret;
}

void ReplicaCommunicator::StartBroadcastInBackGround() {
  is_running_ = true;
  broadcast_thread_ = std::thread([&]() {
    while (IsRunning()) {
      std::vector<std::unique_ptr<QueueItem>> batch_req =
          batch_queue_.Pop(10000);
      if (batch_req.empty()) {
        continue;
      }
      BroadcastData broadcast_data;
      for (auto& queue_item : batch_req) {
        broadcast_data.add_data()->swap(queue_item->data);
      }

      global_stats_->SendBroadCastMsg(broadcast_data.data_size());
      int ret = SendMessageFromPool(broadcast_data, replicas_);
      if (ret < 0) {
        LOG(ERROR) << "broadcast request fail:";
      }
    }
  });
}

int ReplicaCommunicator::SendMessage(const google::protobuf::Message& message) {
  global_stats_->BroadCastMsg();
  if (is_use_long_conn_) {
    auto item = std::make_unique<QueueItem>();
    item->data = NetChannel::GetRawMessageString(message, verifier_);
    batch_queue_.Push(std::move(item));
    return 0;
  } else {
    LOG(ERROR) << "send internal";
    return SendMessageInternal(message, replicas_);
  }
}

int ReplicaCommunicator::SendMessage(const google::protobuf::Message& message,
                                     const ReplicaInfo& replica_info) {
  if (is_use_long_conn_) {
    std::string data = NetChannel::GetRawMessageString(message, verifier_);
    BroadcastData broadcast_data;
    broadcast_data.add_data()->swap(data);
    return SendMessageFromPool(broadcast_data, {replica_info});
  } else {
    return SendMessageInternal(message, {replica_info});
  }
}

int ReplicaCommunicator::SendBatchMessage(
    const std::vector<std::unique_ptr<Request>>& messages,
    const ReplicaInfo& replica_info) {
  if (is_use_long_conn_) {
    BroadcastData broadcast_data;
    for (const auto& message : messages) {
      std::string data = NetChannel::GetRawMessageString(*message, verifier_);
      broadcast_data.add_data()->swap(data);
    }
    return SendMessageFromPool(broadcast_data, {replica_info});
  } else {
    int ret = 0;
    for (const auto& message : messages) {
      ret += SendMessageInternal(*message, {replica_info});
    }
    return ret;
  }
}

int ReplicaCommunicator::SendMessageFromPool(
    const google::protobuf::Message& message,
    const std::vector<ReplicaInfo>& replicas) {
  int ret = 0;
  std::string data;
  message.SerializeToString(&data);
  global_stats_->SendBroadCastMsgPerRep();
  std::lock_guard<std::mutex> lk(mutex_);
  for (const auto& replica : replicas) {
    auto client = GetClientFromPool(replica.ip(), replica.port());
    if (client == nullptr) {
      continue;
    }
    if (client->SendMessage(data) == 0) {
      ret++;
    } else {
      LOG(ERROR) << "send to:" << replica.ip() << " fail";
    }
  }
  return ret;
}

int ReplicaCommunicator::SendMessageInternal(
    const google::protobuf::Message& message,
    const std::vector<ReplicaInfo>& replicas) {
  int ret = 0;
  for (const auto& replica : replicas) {
    auto client = GetClient(replica.ip(), replica.port());
    if (client == nullptr) {
      continue;
    }
    if (verifier_ != nullptr) {
      client->SetSignatureVerifier(verifier_);
    }
    if (client->SendRawMessage(message) == 0) {
      ret++;
    }
  }
  return ret;
}

AsyncReplicaClient* ReplicaCommunicator::GetClientFromPool(
    const std::string& ip, int port) {
  if (client_pools_.find(std::make_pair(ip, port)) == client_pools_.end()) {
    auto client = std::make_unique<AsyncReplicaClient>(
        &io_service_, ip, port + (is_use_long_conn_ ? 10000 : 0), true);
    client_pools_[std::make_pair(ip, port)] = std::move(client);
  }
  return client_pools_[std::make_pair(ip, port)].get();
}

std::unique_ptr<NetChannel> ReplicaCommunicator::GetClient(
    const std::string& ip, int port) {
  return std::make_unique<NetChannel>(ip, port);
}

void ReplicaCommunicator::BroadCast(const google::protobuf::Message& message) {
  int ret = SendMessage(message);
  if (ret < 0) {
    LOG(ERROR) << "broadcast request fail:";
  }
}

void ReplicaCommunicator::SendMessage(const google::protobuf::Message& message,
                                      int64_t node_id) {
  ReplicaInfo target_replica;
  for (const auto& replica : replicas_) {
    if (replica.id() == node_id) {
      target_replica = replica;
      break;
    }
  }
  if (target_replica.ip().empty()) {
    for (const auto& replica : GetClientReplicas()) {
      if (replica.id() == node_id) {
        target_replica = replica;
        break;
      }
    }
  }

  if (target_replica.ip().empty()) {
    LOG(ERROR) << "no replica info node:" << node_id;
    return;
  }

  int ret = SendMessage(message, target_replica);
  if (ret < 0) {
    LOG(ERROR) << "broadcast request fail:";
  }
}

}  // namespace resdb
