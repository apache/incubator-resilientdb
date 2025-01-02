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

#include "platform/consensus/ordering/geo_pbft/geo_pbft_commitment.h"

#include <glog/logging.h>

#include "platform/consensus/ordering/common/transaction_utils.h"

namespace resdb {

GeoPBFTCommitment::GeoPBFTCommitment(
    std::unique_ptr<GeoGlobalExecutor> global_executor,
    const ResDBConfig& config, std::unique_ptr<SystemInfo> system_info,
    ReplicaCommunicator* replica_communicator, SignatureVerifier* verifier)
    : global_executor_(std::move(global_executor)),
      stop_(false),
      config_(std::move(config)),
      system_info_(std::move(system_info)),
      replica_communicator_(std::move(replica_communicator)),
      verifier_(verifier) {
  global_stats_ = Stats::GetGlobalStats();
  executed_thread_ =
      std::thread(&GeoPBFTCommitment::PostProcessExecutedMsg, this);
}

GeoPBFTCommitment::~GeoPBFTCommitment() {
  stop_ = true;
  if (executed_thread_.joinable()) {
    executed_thread_.join();
  }
}

bool GeoPBFTCommitment::VerifyCerts(const BatchUserRequest& request,
                                    const std::string& raw_data) {
  if (verifier_) {
    std::string hash = request.hash();
    for (const auto& sig : request.committed_certs().committed_certs()) {
      bool valid = verifier_->VerifyMessage(hash, sig);
      if (!valid) {
        LOG(ERROR) << "sign is not valid:" << sig.DebugString();
        return false;
      }
    }
  }
  return true;
}

bool GeoPBFTCommitment::AddNewReq(uint64_t seq, uint32_t sender_region) {
  std::lock_guard<std::mutex> lk(mutex_);
  if (seq < min_seq_) {
    return false;
  }
  // LOG(ERROR)<<"add req seq:"<<seq<<" regioin:"<<sender_region;
  auto ret = checklist_[seq % (1 << 20)].insert(sender_region);
  return ret.second;
}

void GeoPBFTCommitment::UpdateSeq(uint64_t seq) {
  std::lock_guard<std::mutex> lk(mutex_);
  if (seq > min_seq_) {
    min_seq_ = seq;
  }
}

int GeoPBFTCommitment::GeoProcessCcm(std::unique_ptr<Context> context,
                                     std::unique_ptr<Request> request) {
  int sender_region_id = request->region_info().region_id();
  if (!AddNewReq(request->seq(), sender_region_id)) {
    // LOG(ERROR) << "geo_request already received, from sender id: "
    //		       << request->sender_id() << " seq:" << request->seq() <<"
    // region:"<<sender_region_id
    //		       << " ,no more action needed.";
    return 1;
  }
  // global_stats_->IncGeoRequest();

  BatchUserRequest batch_request;
  if (!batch_request.ParseFromString(request->data())) {
    LOG(ERROR) << "[GeoGlobalExecutor] parse data fail!";
    return -2;
  }

  if (!batch_request.has_committed_certs() ||
      !VerifyCerts(batch_request, request->data())) {
    // CheckCertificates
    LOG(ERROR) << "no certs";
    return -2;
  }
  // return global_executor_->OrderGeoRequest(std::move(request));

  ResConfigData config_data = config_.GetConfigData();
  int self_region_id = config_data.self_region_id();
  // LOG(ERROR)<<"get request seq:"<<request->seq()<<" from:"<<sender_region_id;
  // if the request comes from another region, do local broadcast
  if (sender_region_id != self_region_id) {
    std::unique_ptr<Request> broadcast_geo_req =
        resdb::NewRequest(Request::TYPE_GEO_REQUEST, *request,
                          config_.GetSelfInfo().id(), sender_region_id);
    // LOG(ERROR) << "[GeoProcessCcm] start broadcasting geo_request locally...
    // ";
    replica_communicator_->BroadCast(*broadcast_geo_req);
  }
  return global_executor_->OrderGeoRequest(std::move(request));
}

int GeoPBFTCommitment::PostProcessExecutedMsg() {
  while (!stop_) {
    auto batch_resp = global_executor_->GetResponseMsg();
    if (batch_resp == nullptr) {
      continue;
    }
    Request request;
    request.set_seq(batch_resp->seq());
    request.set_type(Request::TYPE_RESPONSE);
    request.set_sender_id(config_.GetSelfInfo().id());
    request.set_current_view(batch_resp->current_view());
    request.set_proxy_id(batch_resp->proxy_id());
    UpdateSeq(batch_resp->seq());
    batch_resp->SerializeToString(request.mutable_data());
    replica_communicator_->SendMessage(request, request.proxy_id());
  }
  return 0;
}

}  // namespace resdb
