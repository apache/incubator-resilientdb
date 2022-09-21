#include "server/resdb_replica_client.h"

#include <glog/logging.h>

#include <thread>

#include "proto/broadcast.pb.h"

namespace resdb {

constexpr int BROADCAST_BATCH_NUM = 50;

ResDBReplicaClient::ResDBReplicaClient(const std::vector<ReplicaInfo>& replicas,
                                       SignatureVerifier* verifier,
                                       bool is_use_long_conn, int epoll_num)
    : replicas_(replicas),
      verifier_(verifier),
      is_running_(false),
      batch_queue_("bc_batch", BROADCAST_BATCH_NUM),
      is_use_long_conn_(is_use_long_conn) {
  global_stats_ = Stats::GetGlobalStats();
  if (is_use_long_conn_) {
    worker_ = std::make_unique<boost::asio::io_service::work>(io_service_);
    for (int i = 0; i < epoll_num; ++i) {
      worker_threads_.push_back(std::thread([&]() { io_service_.run(); }));
    }
  }

  StartBroadcastInBackGround();
}

ResDBReplicaClient::~ResDBReplicaClient() {
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

bool ResDBReplicaClient::IsInPool(const ReplicaInfo& replica_info) {
  for (auto& replica : replicas_) {
    if (replica_info.ip() == replica.ip() &&
        replica_info.port() == replica.port()) {
      return true;
    }
  }
  return false;
}

bool ResDBReplicaClient::IsRunning() const { return is_running_; }

void ResDBReplicaClient::UpdateClientReplicas(
    const std::vector<ReplicaInfo>& replicas) {
  clients_ = replicas;
}

std::vector<ReplicaInfo> ResDBReplicaClient::GetClientReplicas() {
  return clients_;
}

int ResDBReplicaClient::SendHeartBeat(const HeartBeatInfo& hb_info) {
  int ret = 0;
  for (const auto& replica : replicas_) {
    ResDBClient client(replica.ip(), replica.port());
    if (client.SendRequest(hb_info, Request::TYPE_HEART_BEAT) == 0) {
      ret++;
    }
  }
  return ret;
}

void ResDBReplicaClient::StartBroadcastInBackGround() {
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

int ResDBReplicaClient::SendMessage(const google::protobuf::Message& message) {
  global_stats_->BroadCastMsg();
  if (is_use_long_conn_) {
    auto item = std::make_unique<QueueItem>();
    item->data = ResDBClient::GetRawMessageString(message, verifier_);
    batch_queue_.Push(std::move(item));
    return 0;
  } else {
    LOG(ERROR) << "send internal";
    return SendMessageInternal(message, replicas_);
  }
}

int ResDBReplicaClient::SendMessage(const google::protobuf::Message& message,
                                    const ReplicaInfo& replica_info) {
  if (is_use_long_conn_) {
    std::string data = ResDBClient::GetRawMessageString(message, verifier_);
    BroadcastData broadcast_data;
    broadcast_data.add_data()->swap(data);
    return SendMessageFromPool(broadcast_data, {replica_info});
  } else {
    return SendMessageInternal(message, {replica_info});
  }
}

int ResDBReplicaClient::SendMessageFromPool(
    const google::protobuf::Message& message,
    const std::vector<ReplicaInfo>& replicas) {
  int ret = 0;
  std::string data;
  message.SerializeToString(&data);
  global_stats_->SendBroadCastMsgPerRep();
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

int ResDBReplicaClient::SendMessageInternal(
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

AsyncReplicaClient* ResDBReplicaClient::GetClientFromPool(const std::string& ip,
                                                          int port) {
  if (client_pools_.find(std::make_pair(ip, port)) == client_pools_.end()) {
    auto client = std::make_unique<AsyncReplicaClient>(
        &io_service_, ip, port + (is_use_long_conn_ ? 10000 : 0), true);
    client_pools_[std::make_pair(ip, port)] = std::move(client);
  }
  return client_pools_[std::make_pair(ip, port)].get();
}

std::unique_ptr<ResDBClient> ResDBReplicaClient::GetClient(
    const std::string& ip, int port) {
  return std::make_unique<ResDBClient>(ip, port);
}

void ResDBReplicaClient::BroadCast(const google::protobuf::Message& message) {
  int ret = SendMessage(message);
  if (ret < 0) {
    LOG(ERROR) << "broadcast request fail:";
  }
}

void ResDBReplicaClient::SendMessage(const google::protobuf::Message& message,
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
    LOG(ERROR) << "no replica info";
    return;
  }

  int ret = SendMessage(message, target_replica);
  if (ret < 0) {
    LOG(ERROR) << "broadcast request fail:";
  }
}

}  // namespace resdb
