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

#include "platform/networkstrate/consensus_manager.h"

#include <glog/logging.h>
#include <unistd.h>

#include "platform/proto/broadcast.pb.h"

namespace resdb {

namespace {

bool ReplicaExisted(const ReplicaInfo& replica_info,
                    const std::vector<ReplicaInfo>& replicas) {
  for (auto& rep : replicas) {
    if (rep.id() == replica_info.id()) {
      return true;
    }
  }
  return false;
}

}  // namespace

ConsensusManager::ConsensusManager(const ResDBConfig& config)
    : config_(config), global_stats_(Stats::GetGlobalStats()) {
  if (config_.SignatureVerifierEnabled()) {
    verifier_ = std::make_unique<SignatureVerifier>(
        config_.GetPrivateKey(), config_.GetPublicKeyCertificateInfo());
  }
  bc_client_ = GetReplicaClient(config_.GetReplicaInfos(), true);
}

ConsensusManager::~ConsensusManager() {
  bc_client_.reset();
  Stop();
}

void ConsensusManager::UpdateBroadCastClient() {
  bc_client_ = GetReplicaClient(GetReplicas(), true);
}

ReplicaCommunicator* ConsensusManager::GetBroadCastClient() {
  return bc_client_.get();
}

SignatureVerifier* ConsensusManager::GetSignatureVerifier() {
  return verifier_ == nullptr ? nullptr : verifier_.get();
}

bool ConsensusManager::IsReady() const { return is_ready_; }

void ConsensusManager::Stop() {
  ServiceInterface::Stop();
  if (heartbeat_thread_.joinable()) {
    heartbeat_thread_.join();
  }
}

void ConsensusManager::Start() {
  ServiceInterface::Start();
  if (config_.HeartBeatEnabled() && verifier_) {
    heartbeat_thread_ =
        std::thread(&ConsensusManager::HeartBeat, this);  // pass by reference
  }
}

// Keep Boardcast the public keys to others.
void ConsensusManager::HeartBeat() {
  LOG(INFO) << "heart beat start";
  int sleep_time = 1;
  std::mutex mutex;
  std::condition_variable cv;
  while (IsRunning()) {
    {
      std::unique_lock<std::mutex> lk(hb_mutex_);
      SendHeartBeat();
    }
    std::unique_lock<std::mutex> lk(mutex);
    cv.wait_for(lk, std::chrono::microseconds(sleep_time * 1000000),
                [&] { return !IsRunning(); });
    if (is_ready_) {
      if (config_.IsTestMode()) {
        sleep_time = 1;
      } else {
        sleep_time = 60;
      }
    }
  }
}

void ConsensusManager::SendHeartBeat() {
  auto keys = verifier_->GetAllPublicKeys();

  std::vector<ReplicaInfo> replicas = GetAllReplicas();
  LOG(ERROR) << "all replicas:" << replicas.size();
  std::vector<ReplicaInfo> client_replicas = GetClientReplicas();
  HeartBeatInfo hb_info;
  hb_info.set_sender(config_.GetSelfInfo().id());
  hb_info.set_ip(config_.GetSelfInfo().ip());
  hb_info.set_port(config_.GetSelfInfo().port());
  hb_info.set_hb_version(version_);
  for (const auto& key : keys) {
    *hb_info.add_public_keys() = key;
    hb_info.add_node_version(hb_[key.public_key_info().node_id()]);
  }
  for (const auto& client : client_replicas) {
    replicas.push_back(client);
  }
  auto client = GetReplicaClient(replicas, false);
  if (client == nullptr) {
    return;
  }

  // If it is not a client node, broadcost the current primary to the client.
  if (config_.GetPublicKeyCertificateInfo()
          .public_key()
          .public_key_info()
          .type() == CertificateKeyInfo::REPLICA) {
    hb_info.set_primary(GetPrimary());
    hb_info.set_version(GetVersion());
  }
  LOG(ERROR) << " server:" << config_.GetSelfInfo().id() << " sends HB"
             << " is ready:" << is_ready_
             << " client size:" << client_replicas.size()
             << " svr size:" << replicas.size();

  Request request;
  request.set_type(Request::TYPE_HEART_BEAT);
  request.mutable_region_info()->set_region_id(
      config_.GetConfigData().self_region_id());
  hb_info.SerializeToString(request.mutable_data());

  int ret = client->SendHeartBeat(request);
  if (ret <= 0) {
    LOG(ERROR) << " server:" << config_.GetSelfInfo().id()
               << " sends HB fail:" << ret;
  }
}

// Porcess the packages received from the network.
// context contains the client socket which can be used for sending response to
// the client, the signature for the request will be filled inside the context
// when parsing the message.
int ConsensusManager::Process(std::unique_ptr<Context> context,
                              std::unique_ptr<DataInfo> request_info) {
  global_stats_->IncClientCall();
  // Decode the whole message, it includes the certificate and data.
  ResDBMessage message;
  if (!message.ParseFromArray(request_info->buff, request_info->data_len)) {
    LOG(ERROR) << "parse data info fail";
    return -1;
  }

  std::unique_ptr<Request> request = std::make_unique<Request>();
  if (!request->ParseFromString(message.data())) {
    LOG(ERROR) << "parse data info fail";
    return -1;
  }

  if (request->type() == Request::TYPE_HEART_BEAT) {
    return Dispatch(std::move(context), std::move(request));
  }

  // Check if the certificate is valid.
  if (message.has_signature() && verifier_) {
    bool valid = verifier_->VerifyMessage(message.data(), message.signature());
    if (!valid) {
      LOG(ERROR) << "request is not valid:"
                 << message.signature().DebugString();
      LOG(ERROR) << " msg:" << message.data().size()
                 << " is recovery:" << request->is_recovery();
      return -2;
    }
  } else {
  }

  // forward the signature to the request so that it can be included in the
  // request/response set if needed.
  context->signature = message.signature();
  // LOG(ERROR) << "======= server:" << config_.GetSelfInfo().id()
  //          << " get request type:" << request->type()
  //         << " from:" << request->sender_id();

  return Dispatch(std::move(context), std::move(request));
}

// Dispatch the request if it is a heart beat message from other replica or a
// cert notification from clients. Otherwise, forward to the worker.
int ConsensusManager::Dispatch(std::unique_ptr<Context> context,
                               std::unique_ptr<Request> request) {
  if (request->type() == Request::TYPE_HEART_BEAT) {
    return ProcessHeartBeat(std::move(context), std::move(request));
  }
  return ConsensusCommit(std::move(context), std::move(request));
}

int ConsensusManager::ProcessHeartBeat(std::unique_ptr<Context> context,
                                       std::unique_ptr<Request> request) {
  std::unique_lock<std::mutex> lk(hb_mutex_);
  std::vector<ReplicaInfo> replicas = GetReplicas();
  HeartBeatInfo hb_info;
  if (!hb_info.ParseFromString(request->data())) {
    LOG(ERROR) << "parse replica info fail\n";
    return -1;
  }

  LOG(ERROR) << "receive public size:" << hb_info.public_keys().size()
             << " primary:" << hb_info.primary()
             << " version:" << hb_info.version()
             << " from region:" << request->region_info().region_id()
             << " sender:" << hb_info.sender()
             << " last send:" << hb_info.hb_version()
             << " current v:" << hb_[hb_info.sender()];

  if (request->region_info().region_id() ==
      config_.GetConfigData().self_region_id()) {
    if (config_.GetPublicKeyCertificateInfo()
            .public_key()
            .public_key_info()
            .type() == CertificateKeyInfo::CLIENT) {
      // TODO count 2f+1 before setting a new primary
      SetPrimary(hb_info.primary(), hb_info.version());
    }
  }

  int replica_num = 0;
  // Update the public keys received from others.
  for (const auto& public_key : hb_info.public_keys()) {
    if (verifier_ && !verifier_->AddPublicKey(public_key)) {
      LOG(ERROR) << "set public key fail from:"
                 << public_key.public_key_info().node_id();
      continue;
    }
    if (request->region_info().region_id() !=
        config_.GetConfigData().self_region_id()) {
      // LOG(ERROR) << "key from other region:"
      //           << request->region_info().region_id();
      continue;
    }

    ReplicaInfo info;
    info.set_ip(public_key.public_key_info().ip());
    info.set_port(public_key.public_key_info().port());
    info.set_id(public_key.public_key_info().node_id());
    if (info.ip().empty()) {
      LOG(ERROR) << "public doesn't have ip, skip";
      continue;
    }
    // Check whether there is a new replica joining.
    // TODO notify new replica
    if (public_key.public_key_info().type() == CertificateKeyInfo::REPLICA) {
      replica_num++;
      if (!ReplicaExisted(info, replicas)) {
        // AddNewReplica(info);
      }
    } else {
      if (!ReplicaExisted(info, clients_)) {
        AddNewClient(info);
      }
    }
  }

  if (!hb_info.ip().empty() && hb_info.hb_version() > 0 &&
      hb_[hb_info.sender()] != hb_info.hb_version()) {
    ReplicaInfo info;
    info.set_ip(hb_info.ip());
    info.set_port(hb_info.port());
    info.set_id(hb_info.sender());
    // bc_client_->Flush(info);
    hb_[hb_info.sender()] = hb_info.hb_version();
    SendHeartBeat();
  }

  if (!is_ready_ && replica_num >= config_.GetMinDataReceiveNum()) {
    LOG(ERROR) << "============ Server " << config_.GetSelfInfo().id()
               << " is ready "
                  "=====================";
    is_ready_ = true;
  }
  return 0;
}

int ConsensusManager::ConsensusCommit(std::unique_ptr<Context> context,
                                      std::unique_ptr<Request> request) {
  return -1;
}

std::vector<ReplicaInfo> ConsensusManager::GetClientReplicas() {
  return clients_;
}

std::vector<ReplicaInfo> ConsensusManager::GetAllReplicas() {
  auto config_data = config_.GetConfigData();
  std::vector<ReplicaInfo> ret;
  for (const auto& r : config_data.region()) {
    for (const auto& replica : r.replica_info()) {
      ret.push_back(replica);
    }
  }
  return ret;
}

void ConsensusManager::BroadCast(const Request& request) {
  int ret = bc_client_->SendMessage(request);
  if (ret < 0) {
    LOG(ERROR) << "broadcast request fail:";
  }
}

void ConsensusManager::SendMessage(const google::protobuf::Message& message,
                                   int64_t node_id) {
  std::vector<ReplicaInfo> replicas = GetReplicas();
  ReplicaInfo target_replica;
  for (const auto& replica : replicas) {
    if (replica.id() == node_id) {
      target_replica = replica;
      break;
    }
  }

  if (target_replica.ip().empty()) {
    return;
  }

  int ret = bc_client_->SendMessage(message, target_replica);
  if (ret < 0) {
    LOG(ERROR) << "broadcast request fail:";
  }
}

std::unique_ptr<ReplicaCommunicator> ConsensusManager::GetReplicaClient(
    const std::vector<ReplicaInfo>& replicas, bool is_use_long_conn) {
  return std::make_unique<ReplicaCommunicator>(
      replicas,
      verifier_ == nullptr || config_.GetConfigData().not_need_signature()
          ? nullptr
          : verifier_.get(),
      is_use_long_conn, config_.GetOutputWorkerNum(), config_.GetTcpBatchNum());
}

void ConsensusManager::AddNewReplica(const ReplicaInfo& info) {}

void ConsensusManager::AddNewClient(const ReplicaInfo& info) {
  clients_.push_back(info);
  bc_client_->UpdateClientReplicas(clients_);
}

void ConsensusManager::SetPrimary(uint32_t primary, uint64_t version) {}

uint32_t ConsensusManager::GetPrimary() { return 1; }

uint32_t ConsensusManager::GetVersion() { return 1; }

}  // namespace resdb
