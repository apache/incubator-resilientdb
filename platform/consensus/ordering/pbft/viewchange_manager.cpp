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

#include "platform/consensus/ordering/pbft/viewchange_manager.h"

#include <glog/logging.h>

#include "common/utils/utils.h"
#include "platform/consensus/ordering/pbft/transaction_utils.h"
#include "platform/proto/viewchange_message.pb.h"

namespace resdb {

ComplaningClients::ComplaningClients()
    : proxy_id(0), is_complaining(false), timeout_length_(10000000) {}

ComplaningClients::ComplaningClients(uint64_t proxy_id)
    : proxy_id(proxy_id), is_complaining(false), timeout_length_(10000000) {}

std::shared_ptr<ViewChangeTimeout> ComplaningClients::SetComplaining(
    std::string hash, uint64_t view) {
  this->complain_state_lock.lock();
  this->is_complaining = true;
  auto info = std::make_shared<ViewChangeTimeout>(
      ViewChangeTimerType::TYPE_COMPLAINT, view, this->proxy_id, hash,
      GetCurrentTime(), this->timeout_length_);
  this->viewchange_timeout_set.insert(hash);
  this->complain_state_lock.unlock();
  return info;
}

uint ComplaningClients::CountViewChangeTimeout(std::string hash) {
  this->complain_state_lock.lock();
  uint value = this->viewchange_timeout_set.count(hash);
  this->complain_state_lock.unlock();
  return value;
}

void ComplaningClients::EraseViewChangeTimeout(std::string hash) {
  this->complain_state_lock.lock();
  this->viewchange_timeout_set.erase(hash);
  this->complain_state_lock.unlock();
}

void ComplaningClients::ReleaseComplaining(std::string hash) {
  this->complain_state_lock.lock();
  this->viewchange_timeout_set.erase(hash);
  this->complain_state_lock.unlock();
}

// A manager to address View change process.
// All stuff here will be addressed in sequential by using mutex
// to make things simplier.
ViewChangeManager::ViewChangeManager(const ResDBConfig& config,
                                     CheckPointManager* checkpoint_manager,
                                     MessageManager* message_manager,
                                     SystemInfo* system_info,
                                     ReplicaCommunicator* replica_communicator,
                                     SignatureVerifier* verifier)
    : config_(config),
      checkpoint_manager_(checkpoint_manager),
      message_manager_(message_manager),
      system_info_(system_info),
      replica_communicator_(replica_communicator),
      verifier_(verifier),
      status_(ViewChangeStatus::NONE),
      started_(false),
      stop_(false) {
  view_change_counter_ = 1;
  global_stats_ = Stats::GetGlobalStats();
  if (config_.GetConfigData().enable_viewchange()) {
    collector_pool_ = message_manager->GetCollectorPool();
    sem_init(&viewchange_timer_signal_, 0, 0);
    server_checking_timeout_thread_ =
        std::thread(&ViewChangeManager::MonitoringViewChangeTimeOut, this);
    checkpoint_state_thread_ =
        std::thread(&ViewChangeManager::MonitoringCheckpointState, this);
  }
}

ViewChangeManager::~ViewChangeManager() {
  checkpoint_manager_->Stop();
  if (server_checking_timeout_thread_.joinable()) {
    server_checking_timeout_thread_.join();
  }
  if (checkpoint_state_thread_.joinable()) {
    checkpoint_state_thread_.join();
  }
}

void ViewChangeManager::MayStart() {
  if (started_) {
    return;
  }
  started_ = true;
  LOG(ERROR) << "MAYSTART";

  if (config_.GetPublicKeyCertificateInfo()
          .public_key()
          .public_key_info()
          .type() == CertificateKeyInfo::CLIENT) {
    LOG(ERROR) << "client type not process view change";
    return;
  }

  checkpoint_manager_->SetTimeoutHandler([&]() {
    // LOG(ERROR) << "checkpoint timeout";
    if (status_ == ViewChangeStatus::NONE) {
      view_change_counter_ = 1;
    } else if (status_ == ViewChangeStatus::READY_NEW_VIEW) {
      // If the new view msg expires after receiving enough view
      // messages, trigger a new primary.
      view_change_counter_++;
    }
    // std::lock_guard<std::mutex> lk(status_mutex_);
    if (ChangeStatue(ViewChangeStatus::READY_VIEW_CHANGE)) {
      SendViewChangeMsg();
      auto viewchange_timer = std::make_shared<ViewChangeTimeout>(
          ViewChangeTimerType::TYPE_VIEWCHANGE, system_info_->GetCurrentView(),
          config_.GetSelfInfo().id(), "null", GetCurrentTime(),
          timeout_length_);
      std::lock_guard<std::mutex> lk(vc_mutex_);
      if (viewchange_timeout_min_heap_[config_.GetSelfInfo().id()].size() <
          config_.GetMaxClientComplaintNum()) {
        viewchange_timeout_min_heap_[config_.GetSelfInfo().id()].push(
            viewchange_timer);
        sem_post(&viewchange_timer_signal_);
      }
    }
  });
}

bool ViewChangeManager::ChangeStatue(ViewChangeStatus status) {
  if (status == ViewChangeStatus::READY_VIEW_CHANGE) {
    if (status_ != ViewChangeStatus::READY_VIEW_CHANGE) {
      LOG(ERROR) << "CHANGE STATUS";
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
    LOG(ERROR) << "stable checkpoint invalid";
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
  std::lock_guard<std::mutex> lk(vc_mutex_);
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
  global_stats_->ChangePrimary(id);
  LOG(ERROR) << "View Change Happened";
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

  LOG(INFO) << "[GP] min_s: " << min_s << " max_seq: " << max_seq;

  std::vector<std::unique_ptr<Request>> redo_request;
  // Resent all the request with the current view number.
  for (auto i = min_s + 1; i <= max_seq; ++i) {
    if (prepared_msg.find(i) == prepared_msg.end()) {
      // for sequence hole, create a new request with empty data and
      // sign by the new primary.
      std::unique_ptr<Request> user_request = resdb::NewRequest(
          Request::TYPE_PRE_PREPARE, Request(), config_.GetSelfInfo().id());
      user_request->set_seq(i);
      user_request->set_current_view(new_view_message.view_number());
      user_request->set_hash("null" + std::to_string(i));

      if (verifier_ && need_sign) {
        std::string data;
        auto signature_or = verifier_->SignMessage(data);
        if (!signature_or.ok()) {
          LOG(ERROR) << "Sign message fail";
          continue;
        }
        *user_request->mutable_data_signature() = *signature_or;
      }
      redo_request.push_back(std::move(user_request));
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
      LOG(ERROR) << "view change message in the new-view message is invalid";
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

  LOG(ERROR) << "min_s: " << min_s << " max_s: " << max_s;

  // Check if all the sequences in the committed list exist.
  for (uint64_t i = min_s + 1; i <= max_s; i++) {
    if (seq_set.find(i) == seq_set.end()) {
      LOG(ERROR) << "Committed msg :" << i << " does exist";
      return -2;
    }
  }

  uint64_t max_seq = seq_set.empty() ? max_s : *(--seq_set.end());

  SetCurrentViewAndNewPrimary(new_view_message.view_number());
  message_manager_->SetNextSeq(max_seq + 1);
  LOG(INFO) << "SetNexSeq: " << max_seq + 1;

  // All is fine.
  for (size_t i = 0; i < request_list.size(); ++i) {
    if (new_view_message.request(i).type() ==
        static_cast<int>(Request::TYPE_PRE_PREPARE)) {
      new_view_message.request(i);
      auto non_proposed_hashes =
          collector_pool_->GetCollector(new_view_message.request(i).seq())
              ->GetAllStoredHash();
      for (auto& hash : non_proposed_hashes) {
        duplicate_manager_->EraseProposed(hash);
      }
      replica_communicator_->SendMessage(new_view_message.request(i),
                                         config_.GetSelfInfo());
    } else {
      if (new_view_message.request(i).seq() >
          checkpoint_manager_->GetHighestPreparedSeq()) {
        checkpoint_manager_->SetHighestPreparedSeq(
            new_view_message.request(i).seq());
      }
      replica_communicator_->BroadCast(new_view_message.request(i));
    }
  }

  ChangeStatue(ViewChangeStatus::NONE);
  return config_.GetSelfInfo().id() == system_info_->GetPrimaryId() ? -4 : 0;
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

  LOG(ERROR) << "ViewChange message from " << request->sender_id();

  size_t request_size = AddRequest(viewchange_message, request->sender_id());
  if (request_size >= config_.GetMinDataReceiveNum()) {
    // process new view
    if (IsNextPrimary(viewchange_message.view_number())) {
      std::lock_guard<std::mutex> lk(mutex_);
      SendNewViewMsg(viewchange_message.view_number());
    } else {
      auto newview_timer = std::make_shared<ViewChangeTimeout>(
          ViewChangeTimerType::TYPE_NEWVIEW, system_info_->GetCurrentView(),
          config_.GetSelfInfo().id(), "null", GetCurrentTime(),
          timeout_length_);
      std::lock_guard<std::mutex> lk(vc_mutex_);
      if (viewchange_timeout_min_heap_[config_.GetSelfInfo().id()].size() <
          config_.GetMaxClientComplaintNum()) {
        viewchange_timeout_min_heap_[config_.GetSelfInfo().id()].push(
            newview_timer);
        sem_post(&viewchange_timer_signal_);
      }
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

  std::lock_guard<std::mutex> lk(vc_mutex_);
  auto requests = viewchange_request_[view_number];

  NewViewMessage new_view_message;
  new_view_message.set_view_number(view_number);

  std::map<uint64_t, std::string> new_view_request;  // <sequence, digest>
  for (auto& it : requests) {
    ViewChangeMessage& msg = it.second;
    LOG(ERROR) << "msg.view_number(): " << msg.view_number()
               << "  view_number: " << view_number << "  sender: " << it.first
               << " msg.stable_ckpt(): " << msg.stable_ckpt().seq();
    msg.set_view_number(view_number);
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

  // Broadcast my new view request.
  std::unique_ptr<Request> request =
      NewRequest(Request::TYPE_NEWVIEW, Request(), config_.GetSelfInfo().id());
  new_view_message.SerializeToString(request->mutable_data());
  replica_communicator_->BroadCast(*request);
}

void ViewChangeManager::SendViewChangeMsg() {
  // PBFT Paper - <VIEW-CHANGE, v + x, n, C, P)
  ViewChangeMessage view_change_message;
  // v + x (view number of the next expected primary)
  view_change_message.set_view_number(system_info_->GetCurrentView() +
                                      view_change_counter_);

  LOG(ERROR) << "current view: " << system_info_->GetCurrentView()
             << "  view number: " << view_change_message.view_number()
             << "  view_change_counter_ " << view_change_counter_;
  assert(view_change_message.view_number() ==
         system_info_->GetCurrentView() + view_change_counter_);

  // n (sequence number of the latest checkpoint) and C (proof for the stable
  // checkpoint)
  *view_change_message.mutable_stable_ckpt() =
      checkpoint_manager_->GetStableCheckpointWithVotes();

  // P - P is a set containing a set Pm for each request m that prepared at i
  // with a sequence number higher than n.
  int max_seq = checkpoint_manager_->GetHighestPreparedSeq();
  LOG(INFO) << "Check prepared or committed txns from "
            << view_change_message.stable_ckpt().seq() + 1 << " to " << max_seq;

  for (int i = view_change_message.stable_ckpt().seq() + 1; i <= max_seq; ++i) {
    // seq i has been prepared or committed.
    if (message_manager_->GetTransactionState(i) >=
        TransactionStatue::READY_COMMIT) {
      std::vector<RequestInfo> proof_info =
          message_manager_->GetPreparedProof(i);
      assert(proof_info.size() >= config_.GetMinDataReceiveNum());
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
  replica_communicator_->BroadCast(*request);
}

void ViewChangeManager::AddComplaintTimer(uint64_t proxy_id, std::string hash) {
  LOG(ERROR) << "ADDING COMPLAINT";
  std::lock_guard<std::mutex> lk(vc_mutex_);
  if (complaining_clients_.count(proxy_id) == 0) {
    complaining_clients_[proxy_id].set_proxy_id(proxy_id);
  }
  auto complaint_ = complaining_clients_[proxy_id].SetComplaining(
      hash, system_info_->GetCurrentView());
  if (viewchange_timeout_min_heap_[proxy_id].size() <
      config_.GetMaxClientComplaintNum()) {
    viewchange_timeout_min_heap_[proxy_id].push(complaint_);
    sem_post(&viewchange_timer_signal_);
  } else {
    // LOG(INFO) << "The number of complaints reaches the maximum value";
  }
}

void ViewChangeManager::MonitoringViewChangeTimeOut() {
  while (!stop_) {
    // [DK3] After timer is out, the client will check if the corresponding
    // client request has recevied sufficient valid responses
    sem_wait(&viewchange_timer_signal_);
    vc_mutex_.lock();
    bool empty = true;
    std::shared_ptr<ViewChangeTimeout> viewchange_timeout;
    for (auto& heap : viewchange_timeout_min_heap_) {
      if (!heap.second.empty()) {
        viewchange_timeout = heap.second.top();
        heap.second.pop();
        vc_mutex_.unlock();
        empty = false;
        break;
      }
    }
    if (empty) {
      vc_mutex_.unlock();
      continue;
    }
    auto current_time = GetCurrentTime();
    if (viewchange_timeout->timeout_time > current_time) {
      usleep(viewchange_timeout->timeout_time - current_time);
    }
    // [DK3] if not enough responses are received, the client broadcasts the
    // client request to all replicas
    if (viewchange_timeout->type == ViewChangeTimerType::TYPE_NEWVIEW) {
      if (status_ == ViewChangeStatus::READY_NEW_VIEW &&
          viewchange_timeout->view == system_info_->GetCurrentView()) {
        // [DK12] if the replicas cannot receive a newview message in a timely
        // manner, they will enter the next view and starts a new round of
        // viewchange. SetCurrentViewAndNewPrimary(viewchange_timeout->view +
        // 1);
        LOG(ERROR) << "It is time to start a new viewchange";
        checkpoint_manager_->TimeoutHandler();
      }
    } else if (viewchange_timeout->type ==
               ViewChangeTimerType::TYPE_VIEWCHANGE) {
      // [DK9] if the primary cannot get enough viewchange messages before the
      // timer is out, then it broadcasts its viewchanges messages and starts
      // the timer again.
      if (status_ == ViewChangeStatus::READY_VIEW_CHANGE &&
          viewchange_timeout->view == system_info_->GetCurrentView()) {
        LOG(ERROR) << "It is time to rebroacast viewchange messages";
        ChangeStatue(ViewChangeStatus::VIEW_CHANGE_FAIL);
        checkpoint_manager_->TimeoutHandler();
      }
    } else if (viewchange_timeout->type ==
               ViewChangeTimerType::TYPE_COMPLAINT) {
      // [DK7] if the primary does not broadcast the request in a timely manner,
      // the replica starts a viewchange
      if (complaining_clients_[viewchange_timeout->proxy_id]
              .CountViewChangeTimeout(viewchange_timeout->hash)) {
        complaining_clients_[viewchange_timeout->proxy_id]
            .EraseViewChangeTimeout(viewchange_timeout->hash);
      }
      std::lock_guard<std::mutex> lk(status_mutex_);
      if (status_ == ViewChangeStatus::NONE &&
          viewchange_timeout->view == system_info_->GetCurrentView()) {
        if (viewchange_timeout->start_time >=
            message_manager_->GetLastCommittedTime(
                viewchange_timeout->proxy_id)) {
          LOG(ERROR) << "It is time to start a viewchange";
          checkpoint_manager_->TimeoutHandler();
          assert(status_ == ViewChangeStatus::READY_VIEW_CHANGE);
        }
      }
    }
  }
}

void ViewChangeManager::MonitoringCheckpointState() {
  uint64_t last_seq_value = 0;
  while (!stop_) {
    sem_wait(checkpoint_manager_->CommitableSeqSignal());
    auto value = checkpoint_manager_->GetCommittableSeq();
    if (last_seq_value != value) {
      last_seq_value = value;
      if (IsInViewChange()) {
        ChangeStatue(ViewChangeStatus::NONE);
      }
    }
  }
}

void ViewChangeManager::SetDuplicateManager(DuplicateManager* manager) {
  duplicate_manager_ = manager;
}

}  // namespace resdb
