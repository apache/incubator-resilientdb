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

#include "server/consensus_service.h"

#include <glog/logging.h>
#include <unistd.h>

#include "proto/broadcast.pb.h"

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

ConsensusService::ConsensusService(const ResDBConfig& config)
    : config_(config), global_stats_(Stats::GetGlobalStats()) {
  if (config_.SignatureVerifierEnabled()) {
    verifier_ = std::make_unique<SignatureVerifier>(
        config_.GetPrivateKey(), config_.GetPublicKeyCertificateInfo());
  }
  bc_client_ = GetReplicaClient(config_.GetReplicaInfos(), true);
}

ConsensusService::~ConsensusService() {
  bc_client_.reset();
  Stop();
}

void ConsensusService::UpdateBroadCastClient() {
  bc_client_ = GetReplicaClient(GetReplicas(), true);
}

ResDBReplicaClient* ConsensusService::GetBroadCastClient() {
  return bc_client_.get();
}

SignatureVerifier* ConsensusService::GetSignatureVerifier() {
  return verifier_ == nullptr ? nullptr : verifier_.get();
}

bool ConsensusService::IsReady() const { return is_ready_; }

void ConsensusService::Stop() {
  ResDBService::Stop();
  if (heartbeat_thread_.joinable()) {
    heartbeat_thread_.join();
  }
}

void ConsensusService::Start() {
  ResDBService::Start();
  if (config_.HeartBeatEnabled() && verifier_) {
    heartbeat_thread_ =
        std::thread(&ConsensusService::HeartBeat, this);  // pass by reference
  }
}

// Keep Boardcast the public keys to others.
void ConsensusService::HeartBeat() {
  LOG(INFO) << "heart beat start";
  int sleep_time = 1;
  std::mutex mutex;
  std::condition_variable cv;
  while (IsRunning()) {
    auto keys = verifier_->GetAllPublicKeys();

    std::vector<ReplicaInfo> replicas = GetAllReplicas();
    LOG(ERROR) << "all replicas:" << replicas.size();
    std::vector<ReplicaInfo> client_replicas = GetClientReplicas();
    HeartBeatInfo hb_info;
    for (const auto& key : keys) {
      *hb_info.add_public_keys() = key;
    }
    for (const auto& client : client_replicas) {
      replicas.push_back(client);
    }
    auto client = GetReplicaClient(replicas, false);
    if (client == nullptr) {
      continue;
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
    std::unique_lock<std::mutex> lk(mutex);
    cv.wait_for(lk, std::chrono::microseconds(sleep_time * 1000000),
                [&] { return !IsRunning(); });
    if (is_ready_) {
      if (config_.IsTestMode()) {
        sleep_time = 1;
      } else {
        sleep_time = 60 * 2;
      }
    }
  }
}

// Porcess the packages received from the network.
// context contains the client socket which can be used for sending response to
// the client, the signature for the request will be filled inside the context
// when parsing the message.
int ConsensusService::Process(std::unique_ptr<Context> context,
                              std::unique_ptr<DataInfo> request_info) {
  global_stats_->IncClientCall();
  // Decode the whole message, it includes the certificate and data.
  ResDBMessage message;
  if (!message.ParseFromArray(request_info->buff, request_info->data_len)) {
    LOG(ERROR) << "parse data info fail";
    return -1;
  }

  // Check if the certificate is valid.
  if (message.has_signature() && verifier_) {
    bool valid = verifier_->VerifyMessage(message.data(), message.signature());
    if (!valid) {
      LOG(ERROR) << "request is not valid:"
                 << message.signature().DebugString();
      LOG(ERROR) << " msg:" << message.data().size();
      return -2;
    }
  } else {
  }

  std::unique_ptr<Request> request = std::make_unique<Request>();
  if (!request->ParseFromString(message.data())) {
    LOG(ERROR) << "parse data info fail";
    return -1;
  }

  std::string tmp;
  if (!request->SerializeToString(&tmp)) {
    return -1;
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
int ConsensusService::Dispatch(std::unique_ptr<Context> context,
                               std::unique_ptr<Request> request) {
  if (request->type() == Request::TYPE_HEART_BEAT) {
    return ProcessHeartBeat(std::move(context), std::move(request));
  }
  return ConsensusCommit(std::move(context), std::move(request));
}

int ConsensusService::ProcessHeartBeat(std::unique_ptr<Context> context,
                                       std::unique_ptr<Request> request) {
  std::vector<ReplicaInfo> replicas = GetReplicas();
  HeartBeatInfo hb_info;
  if (!hb_info.ParseFromString(request->data())) {
    LOG(ERROR) << "parse replica info fail\n";
    return -1;
  }

  LOG(INFO) << "receive public size:" << hb_info.public_keys().size()
            << " primary:" << hb_info.primary()
            << " version:" << hb_info.version()
            << " from region:" << request->region_info().region_id();

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
  if (!is_ready_ && replica_num >= config_.GetMinDataReceiveNum()) {
    LOG(ERROR) << "============ Server " << config_.GetSelfInfo().id()
               << " is ready "
                  "=====================";
    is_ready_ = true;
  }
  return 0;
}

int ConsensusService::ConsensusCommit(std::unique_ptr<Context> context,
                                      std::unique_ptr<Request> request) {
  return -1;
}

std::vector<ReplicaInfo> ConsensusService::GetClientReplicas() {
  return clients_;
}

std::vector<ReplicaInfo> ConsensusService::GetAllReplicas() {
  auto config_data = config_.GetConfigData();
  std::vector<ReplicaInfo> ret;
  for (const auto& r : config_data.region()) {
    for (const auto& replica : r.replica_info()) {
      ret.push_back(replica);
    }
  }
  return ret;
}

void ConsensusService::BroadCast(const Request& request) {
  int ret = bc_client_->SendMessage(request);
  if (ret < 0) {
    LOG(ERROR) << "broadcast request fail:";
  }
}

void ConsensusService::SendMessage(const google::protobuf::Message& message,
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

std::unique_ptr<ResDBReplicaClient> ConsensusService::GetReplicaClient(
    const std::vector<ReplicaInfo>& replicas, bool is_use_long_conn) {
  return std::make_unique<ResDBReplicaClient>(
      replicas,
      verifier_ == nullptr || config_.GetConfigData().not_need_signature()
          ? nullptr
          : verifier_.get(),
      is_use_long_conn, config_.GetOutputWorkerNum(), config_.GetTcpBatchNum());
}

void ConsensusService::AddNewReplica(const ReplicaInfo& info) {}

void ConsensusService::AddNewClient(const ReplicaInfo& info) {
  clients_.push_back(info);
  bc_client_->UpdateClientReplicas(clients_);
}

void ConsensusService::SetPrimary(uint32_t primary, uint64_t version) {}

uint32_t ConsensusService::GetPrimary() { return 1; }

uint32_t ConsensusService::GetVersion() { return 1; }

}  // namespace resdb
