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

#include "platform/consensus/ordering/poe/viewchange.h"

#include <glog/logging.h>

#include "platform/consensus/ordering/poe/poe_utils.h"

namespace resdb {
namespace poe {

ViewChange::ViewChange(const ResDBConfig& config,
                       MessageManager* message_manager, Commitment* commitment,
                       SystemInfo* system_info,
                       ReplicaCommunicator* replica_communicator,
                       SignatureVerifier* verifier)
    : config_(config),
      is_stop_(false),
      message_manager_(message_manager),
      commitment_(commitment),
      system_info_(system_info),
      replica_communicator_(replica_communicator),
      verifier_(verifier),
      vc_count_(1) {
  if (config_.GetPublicKeyCertificateInfo()
          .public_key()
          .public_key_info()
          .type() == CertificateKeyInfo::CLIENT) {
    return;
  }
  message_manager_->SetExexecutedCallBack([&]() {
    has_new_req_ = true;
    is_inited_ = true;
    Notify();
  });
  is_inited_ = false;
  has_new_req_ = false;
  // monitor_thread_ = std::thread(&ViewChange::Monitor, this);
}

ViewChange::~ViewChange() {
  is_stop_ = true;
  if (monitor_thread_.joinable()) {
    monitor_thread_.join();
  }
}

QC ViewChange::GetQC(uint64_t view) {
  QC qc;
  for (const auto& request : new_view_requests_[view]) {
    *qc.add_signatures() = request->view_signature();
  }
  return qc;
}

bool ViewChange::VerifyTS(const NewViewChangeMsg& request) {
  if (verifier_ == nullptr) {
    return true;
  }
  if (request.qc().signatures_size() < config_.GetMinDataReceiveNum()) {
    LOG(ERROR) << "signature is not enough:" << request.qc().signatures_size();
    return false;
  }
  for (const auto& signature : request.qc().signatures()) {
    bool valid =
        verifier_->VerifyMessage(std::to_string(request.view()), signature);
    if (!valid) {
      LOG(ERROR) << "TS is not valid";
      return false;
    }
  }
  return true;
}

bool ViewChange::VerifyNodeSignagure(const POERequest& request) {
  if (verifier_ == nullptr) {
    return true;
  }

  if (!request.has_data_signature()) {
    LOG(ERROR) << "node signature is empty";
    return false;
  }
  // check signatures
  bool valid =
      verifier_->VerifyMessage(request.hash(), request.data_signature());
  if (!valid) {
    LOG(ERROR) << "signature is not valid:"
               << request.data_signature().DebugString();
    return false;
  }
  return true;
}

void ViewChange::Notify() {
  std::lock_guard<std::mutex> lk(view_change_mutex_);
  view_change_cv_.notify_all();
}

void ViewChange::WaitOrStop() {
  // LOG(ERROR) << "wait vc:" <<
  // config_.GetConfigData().view_change_timeout_ms();
  std::unique_lock<std::mutex> lk(view_change_mutex_);
  view_change_cv_.wait_for(
      lk,
      std::chrono::microseconds(
          config_.GetConfigData().view_change_timeout_ms()),
      [&] { return is_stop_ || has_new_req_; });
}

void ViewChange::Monitor() {
  while (!is_stop_) {
    uint64_t current_view = system_info_->GetCurrentView() + vc_count_;
    WaitOrStop();
    if (is_stop_) {
      break;
    }
    // LOG(ERROR) << " change vc has new req:" << has_new_req_
    //           << " init:" << is_inited_;
    if (has_new_req_ || !is_inited_) {
      has_new_req_ = false;
      continue;
    }
    // LOG(ERROR) << " timeout, trigger vc";
    if (current_view != system_info_->GetCurrentView() + vc_count_) {
      LOG(ERROR) << " view has been changed, skip:" << current_view
                 << " now:" << system_info_->GetCurrentView();
      continue;
    }
    TriggerViewChange(system_info_->GetCurrentView() + vc_count_);
  }
}

void ViewChange::TriggerViewChange(uint64_t new_view) {
  std::unique_lock<std::mutex> lk(view_change_mutex_);
  if (system_info_->GetCurrentView() >= new_view) {
    LOG(ERROR) << " current view:" << system_info_->GetCurrentView()
               << " is higher than the wanted one:" << new_view;
    return;
  }
  message_manager_->LockView(system_info_->GetCurrentView());
  CertifyRequests certify_requests = message_manager_->GetCertifyRequests();
  ViewChangeMsg msg;
  *msg.mutable_certify_requests() = certify_requests;
  msg.set_view(new_view);
  msg.set_sender_id(config_.GetSelfInfo().id());

  if (verifier_) {
    auto signature_or = verifier_->SignMessage(std::to_string(msg.view()));
    if (!signature_or.ok()) {
      LOG(ERROR) << "Sign message fail";
      return;
    }
    *msg.mutable_view_signature() = *signature_or;
  }

  auto request = std::make_unique<Request>();
  msg.SerializeToString(request->mutable_data());
  request->set_type(Request::TYPE_VIEWCHANGE);
  LOG(ERROR) << "send vc request:" << new_view
             << " req size:" << certify_requests.certify_requests_size()
             << " count:" << vc_count_;
  vc_count_++;
  replica_communicator_->BroadCast(*request);
}

void ViewChange::SendNewView(uint64_t new_view) {
  NewViewChangeMsg msg;

  // Obtain the valid certify requets.
  std::map<int64_t, POERequest> certify_reqs;
  for (const auto& request : new_view_requests_[new_view]) {
    for (const auto& vc_request :
         request->certify_requests().certify_requests()) {
      if (certify_reqs.find(vc_request.seq()) != certify_reqs.end()) {
        continue;
      }
      if (VerifyNodeSignagure(vc_request)) {
        certify_reqs[vc_request.seq()] = vc_request;
      }
    }
  }
  for (uint64_t seq = 1;; seq++) {
    if (certify_reqs.find(seq) == certify_reqs.end()) {
      break;
    }
    *msg.mutable_certify_requests()->add_certify_requests() = certify_reqs[seq];
  }

  auto request = std::make_unique<Request>();
  *msg.mutable_qc() = GetQC(new_view);
  msg.set_view(new_view);
  msg.set_sender_id(config_.GetSelfInfo().id());
  msg.SerializeToString(request->mutable_data());

  request->set_type(Request::TYPE_NEWVIEW);
  LOG(ERROR) << "Send vc to primary:"
             << PrimaryId(new_view, config_.GetReplicaInfos().size());
  replica_communicator_->BroadCast(*request);
}

int ViewChange::ProcessViewChange(std::unique_ptr<ViewChangeMsg> vc_msg) {
  std::unique_lock<std::mutex> lk(mutex_);
  uint64_t new_view = vc_msg->view();
  LOG(INFO) << "recv vc request view:" << new_view
            << " from:" << vc_msg->sender_id()
            << " total:" << new_view_msgs_[new_view].size();
  if (new_view_msgs_[new_view].find(vc_msg->sender_id()) !=
      new_view_msgs_[new_view].end()) {
    return 0;
  }
  if (new_view <= system_info_->GetCurrentView()) {
    LOG(ERROR) << "new view:" << new_view << " out dated";
    return -2;
  }
  new_view_msgs_[new_view].insert(vc_msg->sender_id());
  new_view_requests_[new_view].push_back(std::move(vc_msg));
  // f+1
  if (new_view_msgs_[new_view].size() ==
      static_cast<size_t>(config_.GetMaxMaliciousReplicaNum()) + 1) {
    if (new_view_msgs_[new_view].find(config_.GetSelfInfo().id()) ==
        new_view_msgs_[new_view].end()) {
      // received f+1 vc msg.
      TriggerViewChange(new_view);
    }
  }
  // 2f+1
  if (new_view_msgs_[new_view].size() ==
      static_cast<size_t>(config_.GetMinDataReceiveNum())) {
    if (PrimaryId(new_view, config_.GetReplicaInfos().size()) ==
        config_.GetSelfInfo().id()) {
      // SendNewView
      SendNewView(new_view);
    }
  }
  return 0;
}

int ViewChange::ProcessNewView(std::unique_ptr<NewViewChangeMsg> new_vc) {
  LOG(INFO) << " process new view:" << new_vc->view();
  if (!VerifyTS(*new_vc)) {
    LOG(ERROR) << "ts is not valid";
    return -2;
  }
  if (PrimaryId(new_vc->view(), config_.GetReplicaInfos().size()) !=
      new_vc->sender_id()) {
    LOG(ERROR) << " not from primary";
    return -2;
  }

  std::unique_lock<std::mutex> lk(mutex_);
  if (system_info_->GetCurrentView() > new_vc->view()) {
    LOG(ERROR) << "view:" << new_vc->view() << " is outdated";
    return -2;
  }
  std::vector<POERequest> certify_requests;
  uint64_t max_seq = 0;
  for (uint64_t i = 0; i < new_vc->certify_requests().certify_requests_size();
       ++i) {
    if (new_vc->certify_requests().certify_requests(i).seq() != i + 1) {
      LOG(ERROR) << " seq not valid";
      return -2;
    }
    if (!VerifyNodeSignagure(new_vc->certify_requests().certify_requests(i))) {
      LOG(ERROR) << " vc msg not valid";
      return -2;
    }
    certify_requests.push_back(new_vc->certify_requests().certify_requests(i));
    max_seq =
        std::max(max_seq, new_vc->certify_requests().certify_requests(i).seq());
  }
  if (system_info_->GetCurrentView() > new_vc->view()) {
    LOG(ERROR) << " view update to a larger one:" << new_vc->view();
    system_info_->SetCurrentView(new_vc->view());
  }

  for (auto& req : certify_requests) {
    if (message_manager_->IsCommitted(req.seq())) {
      continue;
    }
    LOG(INFO) << " recommit seq:" << req.seq();
    req.set_view(new_vc->view());
    message_manager_->CommitInternal(req, req.data());
  }
  message_manager_->RollBack(max_seq + 1);
  commitment_->Reset(max_seq + 1);
  system_info_->SetCurrentView(new_vc->view());
  vc_count_ = 1;
  message_manager_->UnLockView();
  LOG(INFO) << " go to new view:" << new_vc->view();
  return 0;
}

}  // namespace poe
}  // namespace resdb
