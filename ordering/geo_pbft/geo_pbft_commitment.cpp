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

#include "ordering/geo_pbft/geo_pbft_commitment.h"

#include <glog/logging.h>

#include "ordering/pbft/transaction_utils.h"

namespace resdb {

GeoPBFTCommitment::GeoPBFTCommitment(
    std::unique_ptr<GeoGlobalExecutor> global_executor,
    const ResDBConfig& config, std::unique_ptr<SystemInfo> system_info,
    ResDBReplicaClient* replica_client, SignatureVerifier* verifier)
    : global_executor_(std::move(global_executor)),
      stop_(false),
      config_(std::move(config)),
      system_info_(std::move(system_info)),
      replica_client_(std::move(replica_client)),
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

bool GeoPBFTCommitment::VerifyCerts(const BatchClientRequest& request,
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

  BatchClientRequest batch_request;
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
    replica_client_->BroadCast(*broadcast_geo_req);
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
    replica_client_->SendMessage(request, request.proxy_id());
  }
  return 0;
}

}  // namespace resdb
