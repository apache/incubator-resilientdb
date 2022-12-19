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

#include "ordering/pbft/viewchange_manager.h"

#include "glog/logging.h"
#include "ordering/pbft/transaction_utils.h"
#include "proto/viewchange_message.pb.h"

namespace resdb {

// A manager to address View change process.
// All stuff here will be addressed in sequential by using mutex
// to make things simplier.
ViewChangeManager::ViewChangeManager(const ResDBConfig& config,
                                     CheckPointManager* checkpoint_manager,
                                     TransactionManager* transaction_manager,
                                     SystemInfo* system_info,
                                     ResDBReplicaClient* replica_client,
                                     SignatureVerifier* verifier)
    : config_(config),
      checkpoint_manager_(checkpoint_manager),
      transaction_manager_(transaction_manager),
      system_info_(system_info),
      replica_client_(replica_client),
      verifier_(verifier),
      status_(ViewChangeStatus::NONE),
      started_(false) {
  view_change_counter_ = 1;
}

ViewChangeManager::~ViewChangeManager() {}

void ViewChangeManager::MayStart() {
  if (started_) {
    return;
  }
  started_ = true;

  if (config_.GetPublicKeyCertificateInfo()
          .public_key()
          .public_key_info()
          .type() == CertificateKeyInfo::CLIENT) {
    LOG(ERROR) << "client type not process view change";
    return;
  }

  checkpoint_manager_->SetTimeoutHandler([&]() {
    LOG(ERROR) << "checkpoint timeout";
    if (status_ == ViewChangeStatus::NONE) {
      view_change_counter_ = 1;
    } else if (status_ == ViewChangeStatus::READY_NEW_VIEW) {
      // If the new view msg expires after receiving enough view
      // messages, trigger a new primary.
      view_change_counter_++;
    }
    std::lock_guard<std::mutex> lk(status_mutex_);
    if (ChangeStatue(ViewChangeStatus::READY_VIEW_CHANGE)) {
      SendViewChangeMsg();
    }
  });
}

bool ViewChangeManager::ChangeStatue(ViewChangeStatus status) {
  if (status == ViewChangeStatus::READY_VIEW_CHANGE) {
    if (status_ == ViewChangeStatus::NONE) {
      status_ = status;
    }
  } else {
    status_ = status;
  }
  return status_ == status;
}

bool ViewChangeManager::IsInViewChange() {
  return status_ != ViewChangeStatus::NONE;
}

bool ViewChangeManager::IsValidViewChangeMsg(
    const ViewChangeMessage& view_change_message) {
  if (view_change_message.view_number() <= system_info_->GetCurrentView()) {
    LOG(ERROR) << "View number is not greater than current view:"
               << view_change_message.view_number() << "("
               << system_info_->GetCurrentView() << ")";
    return false;
  }

  if (!checkpoint_manager_->IsValidCheckpointProof(
          view_change_message.stable_ckpt())) {
    LOG(ERROR) << "stable checkpoint not invalid";
    return false;
  }

  uint64_t stable_seq = view_change_message.stable_ckpt().seq();

  for (const auto& prepared_msg : view_change_message.prepared_msg()) {
    if (prepared_msg.seq() <= stable_seq) {
      continue;
    }
    // If there is less than 2f+1 proof, reject.
    if (prepared_msg.proof_size() < config_.GetMinDataReceiveNum()) {
      LOG(ERROR) << "proof[" << prepared_msg.proof_size()
                 << "] not enough:" << config_.GetMinDataReceiveNum();
      return false;
    }
    for (const auto& proof : prepared_msg.proof()) {
      if (proof.request().seq() != prepared_msg.seq()) {
        LOG(ERROR) << "proof seq not match";
        return false;
      }
      std::string data;
      proof.request().SerializeToString(&data);
      if (!verifier_->VerifyMessage(data, proof.signature())) {
        LOG(ERROR) << "proof signature not valid";
        return false;
      }
    }
  }
  return true;
}

uint32_t ViewChangeManager::AddRequest(
    const ViewChangeMessage& viewchange_message, uint32_t sender) {
  viewchange_request_[viewchange_message.view_number()][sender] =
      viewchange_message;
  return viewchange_request_[viewchange_message.view_number()].size();
}

bool ViewChangeManager::IsNextPrimary(uint64_t view_number) {
  std::lock_guard<std::mutex> lk(mutex_);
  const std::vector<ReplicaInfo>& replicas = config_.GetReplicaInfos();
  return config_.GetReplicaInfos()[(view_number - 1) % replicas.size()].id() ==
         config_.GetSelfInfo().id();
}

void ViewChangeManager::SetCurrentViewAndNewPrimary(uint64_t view_number) {
  system_info_->SetCurrentView(view_number);

  const std::vector<ReplicaInfo>& replicas = config_.GetReplicaInfos();
  uint32_t id =
      config_.GetReplicaInfos()[(view_number - 1) % replicas.size()].id();
  system_info_->SetPrimary(id);
}

std::vector<std::unique_ptr<Request>> ViewChangeManager::GetPrepareMsg(
    const NewViewMessage& new_view_message, bool need_sign) {
  std::map<uint64_t, Request> prepared_msg;  // <sequence, digest>
  for (const auto& msg : new_view_message.viewchange_messages()) {
    for (const auto& msg : msg.prepared_msg()) {
      uint64_t seq = msg.seq();
      prepared_msg[seq] = msg.proof(0).request();
    }
  }

  uint64_t min_s = std::numeric_limits<uint64_t>::max();
  for (const auto& msg : new_view_message.viewchange_messages()) {
    min_s = std::min(min_s, msg.stable_ckpt().seq());
  }

  uint64_t max_seq = 1;
  if (prepared_msg.size() > 0) {
    max_seq = (--prepared_msg.end())->first;
  }

  std::vector<std::unique_ptr<Request>> redo_request;
  // Resent all the request with the current view number.
  for (auto i = min_s + 1; i <= max_seq; ++i) {
    if (prepared_msg.find(i) == prepared_msg.end()) {
      // for sequence hole, create a new request with empty data and
      // sign by the new primary.
      std::unique_ptr<Request> client_request = resdb::NewRequest(
          Request::TYPE_PRE_PREPARE, Request(), config_.GetSelfInfo().id());
      client_request->set_seq(i);
      client_request->set_current_view(new_view_message.view_number());

      if (verifier_ && need_sign) {
        std::string data;
        auto signature_or = verifier_->SignMessage(data);
        if (!signature_or.ok()) {
          LOG(ERROR) << "Sign message fail";
          continue;
        }
        *client_request->mutable_data_signature() = *signature_or;
      }
      redo_request.push_back(std::move(client_request));
    } else {
      std::unique_ptr<Request> commit_request = resdb::NewRequest(
          Request::TYPE_COMMIT, prepared_msg[i], config_.GetSelfInfo().id());
      commit_request->set_seq(i);
      commit_request->set_current_view(new_view_message.view_number());
      redo_request.push_back(std::move(commit_request));
    }
  }

  return redo_request;
}

int ViewChangeManager::ProcessNewView(std::unique_ptr<Context> context,
                                      std::unique_ptr<Request> request) {
  NewViewMessage new_view_message;
  if (!new_view_message.ParseFromString(request->data())) {
    LOG(ERROR) << "Parsing new_view_msg failed.";
    return -2;
  }
  LOG(INFO) << "Received NEW-VIEW for view " << new_view_message.view_number();
  // Check if new view is from next expected primary
  if (new_view_message.view_number() !=
      system_info_->GetCurrentView() + view_change_counter_) {
    LOG(ERROR) << "View number " << new_view_message.view_number()
               << " is not the same as expected: "
               << system_info_->GetCurrentView() + 1;
    return -2;
  }

  uint64_t min_s = std::numeric_limits<uint64_t>::max(), max_s = 0;
  // Verify each view change message as the new primary does.
  for (const auto& msg : new_view_message.viewchange_messages()) {
    min_s = std::min(min_s, msg.stable_ckpt().seq());
    max_s = std::max(max_s, msg.stable_ckpt().seq());
    if (!IsValidViewChangeMsg(msg)) {
      LOG(ERROR) << "view change message is not invalid";
      return -2;
    }
  }

  // check the re-calculated request is the same as the one in the request.
  auto request_list = GetPrepareMsg(new_view_message, false);
  if (request_list.size() !=
      static_cast<size_t>(new_view_message.request_size())) {
    LOG(ERROR) << "redo request list size not match:" << request_list.size()
               << "- " << new_view_message.request_size();
    return -2;
  }

  std::set<uint64_t> seq_set;
  // only check the data.
  for (size_t i = 0; i < request_list.size(); ++i) {
    if (request_list[i]->data() != new_view_message.request(i).data()) {
      LOG(ERROR) << "data not match";
      return -2;
    }
    seq_set.insert(request_list[i]->seq());
  }

  // Check if all the sequences in the committed list exist.
  for (uint64_t i = min_s + 1; i <= max_s; i++) {
    if (seq_set.find(i) == seq_set.end()) {
      LOG(ERROR) << "Committed msg :" << i << " does exist";
      return -2;
    }
  }

  uint64_t max_seq = *(--seq_set.end());
  SetCurrentViewAndNewPrimary(new_view_message.view_number());
  transaction_manager_->SetNextSeq(max_seq + 1);

  // All is fine.
  for (size_t i = 0; i < request_list.size(); ++i) {
    LOG(ERROR) << " bc seq:" << request_list[i]->seq();
    if (new_view_message.request(i).type() ==
        static_cast<int>(Request::TYPE_PRE_PREPARE)) {
      replica_client_->SendMessage(new_view_message.request(i),
                                   config_.GetSelfInfo());
    } else {
      replica_client_->BroadCast(new_view_message.request(i));
    }
  }

  ChangeStatue(ViewChangeStatus::NONE);
  return 0;
}

int ViewChangeManager::ProcessViewChange(std::unique_ptr<Context> context,
                                         std::unique_ptr<Request> request) {
  ViewChangeMessage viewchange_message;
  if (!viewchange_message.ParseFromString(request->data())) {
    LOG(ERROR) << "pase view change data fail";
    return -2;
  }

  if (!IsValidViewChangeMsg(viewchange_message)) {
    LOG(ERROR) << "view change msg not valid from:" << request->sender_id();
    return -2;
  }

  size_t request_size = AddRequest(viewchange_message, request->sender_id());
  if (request_size >= config_.GetMinClientReceiveNum()) {
    // process new view
    if (IsNextPrimary(viewchange_message.view_number())) {
      std::lock_guard<std::mutex> lk(mutex_);
      SendNewViewMsg(viewchange_message.view_number());
    }
    ChangeStatue(ViewChangeStatus::READY_NEW_VIEW);
  }
  return 0;
}

void ViewChangeManager::SendNewViewMsg(uint64_t view_number) {
  if (new_view_is_sent_) {
    return;
  }
  new_view_is_sent_ = true;

  // PBFT Paper - Primary determines the sequence number min-s of the latest
  // stable checkpoint in V and the highest sequence number max-s in a prepare
  // message in V
  // uint64_t min_s = std::numeric_limits<uint64_t>::max();
  auto requests = viewchange_request_[view_number];

  NewViewMessage new_view_message;
  new_view_message.set_view_number(view_number);

  std::map<uint64_t, std::string> new_view_request;  // <sequence, digest>
  for (const auto& it : requests) {
    const ViewChangeMessage& msg = it.second;
    *new_view_message.add_viewchange_messages() = msg;
  }

  // Get the redo message from the primary. This could not be done
  // in each replica because the primary has to signed some empty
  // request if needed.

  std::vector<std::unique_ptr<Request>> request_list =
      GetPrepareMsg(new_view_message);
  for (const auto& request : request_list) {
    *new_view_message.add_request() = *request;
  }

  // Broadcast my view change request.
  std::unique_ptr<Request> request =
      NewRequest(Request::TYPE_NEWVIEW, Request(), config_.GetSelfInfo().id());
  new_view_message.SerializeToString(request->mutable_data());
  replica_client_->BroadCast(*request);
}

void ViewChangeManager::SendViewChangeMsg() {
  // PBFT Paper - <VIEW-CHANGE, v + x, n, C, P)
  ViewChangeMessage view_change_message;
  // v + x (view number of the next expected primary)
  view_change_message.set_view_number(system_info_->GetCurrentView() +
                                      view_change_counter_);

  // n (sequence number of the latest checkpoint) and C (proof for the stable
  // checkpoint)
  *view_change_message.mutable_stable_ckpt() =
      checkpoint_manager_->GetStableCheckpointWithVotes();

  // P - P is a set containing a set Pm for each request m that prepared at i
  // with a sequence number higher than n.
  int max_seq = checkpoint_manager_->GetMaxTxnSeq();
  for (int i = view_change_message.stable_ckpt().seq() + 1; i <= max_seq; ++i) {
    // seq i has been prepared or committed.
    if (transaction_manager_->GetTransactionState(i) >=
        TransactionStatue::READY_COMMIT) {
      std::vector<RequestInfo> proof_info =
          transaction_manager_->GetPreparedProof(i);
      auto txn = view_change_message.add_prepared_msg();
      txn->set_seq(i);
      for (const auto& info : proof_info) {
        auto proof = txn->add_proof();
        *proof->mutable_request() = *info.request;
        *proof->mutable_signature() = info.signature;
      }
    }
  }

  // Broadcast my view change request.
  std::unique_ptr<Request> request = NewRequest(
      Request::TYPE_VIEWCHANGE, Request(), config_.GetSelfInfo().id());
  view_change_message.SerializeToString(request->mutable_data());
  replica_client_->BroadCast(*request);
}

}  // namespace resdb
