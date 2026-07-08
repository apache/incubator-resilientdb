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

#include "platform/consensus/ordering/zzy/framework/performance_manager.h"

#include <glog/logging.h>
#include <vector>

#include "common/utils/utils.h"

namespace resdb {
namespace zzy {

using comm::CollectorResultCode;

namespace {

std::string BuildPrepareDigest(int view, int64_t seq, const std::string& hash,
                               int replica) {
  return std::to_string(view) + ":" + std::to_string(seq) + ":" + hash + ":" +
         std::to_string(replica);
}

}  // namespace

int ZZYPerformanceManager::Broadcast(int type, const google::protobuf::Message& msg) {
  Request request;
  msg.SerializeToString(request.mutable_data());
  request.set_type(Request::TYPE_CUSTOM_CONSENSUS);
  request.set_user_type(type);
  request.set_sender_id(config_.GetSelfInfo().id());

  replica_communicator_->BroadCast(request);
  return 0;
}

ZZYPerformanceManager::ZZYPerformanceManager(
    const ResDBConfig& config, ReplicaCommunicator* replica_communicator,
    SignatureVerifier* verifier)
    : PerformanceManager(config, replica_communicator, verifier) {
  total_replicas_ = config_.GetReplicaNum();
  f_ = (total_replicas_ - 1) / 3;
  start_seq_ = 1;
  time_limit_ = 30000000;
  ready_thread_ = std::thread(&ZZYPerformanceManager::WaitComplete, this);
}

ZZYPerformanceManager::~ZZYPerformanceManager() {
  stop_ = true;
  if (ready_thread_.joinable()) {
    ready_thread_.join();
  }
}

void ZZYPerformanceManager::AddPkg(int64_t local_id,
                                   std::unique_ptr<BatchUserResponse> resp) {
  std::unique_lock<std::mutex> lk(n_mutex_);
  if (done_.find(local_id) != done_.end()) {
    return;
  }
  done_[local_id] = GetCurrentTime();
  std::unique_lock<std::mutex> rlk(resp_mutex_[local_id % 1000]);
  resp_[local_id % 1000][local_id] = std::move(resp);
}

void ZZYPerformanceManager::PkgDone(int local_id) {
  if (local_id < start_seq_) {
    return;
  }
  done_[local_id] = 0;
}

bool ZZYPerformanceManager::IsTimeout() {
  if (done_.begin()->second == 0) {
    return false;
  }
  return GetCurrentTime() - done_.begin()->second > time_limit_;
}

bool ZZYPerformanceManager::TimeoutLeft() {
  return time_limit_ - GetCurrentTime() - done_.begin()->second > time_limit_;
}

bool ZZYPerformanceManager::Ready() {
  if (done_.empty() || done_.begin()->first != start_seq_) {
    return false;
  }
  if (done_.begin()->second == 0 || IsTimeout()) {
    return true;
  }
  return false;
}

int ZZYPerformanceManager::CheckReady() {
  while (Ready()) {
    if (IsTimeout()) {
      LOG(ERROR) << "check timeout seq:" << start_seq_
                 << " start:" << done_.begin()->second
                 << " end:" << GetCurrentTime()
                 << " diff:" << GetCurrentTime() - done_.begin()->second;
      SendTimeout(start_seq_);
    }
    done_.erase(done_.begin());
    start_seq_++;
  }
  return 0;
}

void ZZYPerformanceManager::WaitComplete() {
  while (!stop_) {
    std::unique_lock<std::mutex> lk(n_mutex_);
    vote_cv_.wait_for(lk, std::chrono::microseconds(10000),
                      [&] { return Ready(); });
    CheckReady();
  }
}

void ZZYPerformanceManager::Notify(int seq) {
  const int id = seq % 1000;
  std::unique_ptr<BatchUserResponse> resp = nullptr;
  {
    std::unique_lock<std::mutex> lk(resp_mutex_[id]);
    if (resp_[id].find(seq) == resp_[id].end()) {
      LOG(ERROR) << "notify seq has done:" << seq;
      return;
    }

    resp = std::move(resp_[id][seq]);
    resp_[id].erase(resp_[id].find(seq));
  }
  SendResponseToClient(*resp);
  std::unique_lock<std::mutex> lk(n_mutex_);
  if (seq < start_seq_) {
    return;
  }
  PkgDone(seq);
  vote_cv_.notify_one();
}

bool ZZYPerformanceManager::ReceiveCommitACK(
    std::unique_ptr<Proposal> proposal) {
  const int64_t seq = proposal->seq();
  const int proposer = proposal->proposer();
  const int local_id = proposal->local_id();
  bool done = false;
  {
    std::unique_lock<std::mutex> lk(c_mutex_[seq % 1000]);
    commit_receive_[seq % 1000][seq].insert(proposer);
    // Zyzzyva slow path: client completes after 2f+1 commit acks.
    if (static_cast<int>(commit_receive_[seq % 1000][seq].size()) >= 2 * f_ + 1) {
      done = true;
    }
  }
  if (done) {
    Notify(local_id);
  }
  return true;
}

int ZZYPerformanceManager::ProcessResponseMsg(std::unique_ptr<Context> context,
                                              std::unique_ptr<Request> request) {
  std::unique_ptr<BatchUserResponse> batch_response =
      std::make_unique<BatchUserResponse>();
  if (!batch_response->ParseFromString(request->data())) {
    LOG(ERROR) << "parse response fail:" << request->data().size()
               << " seq:" << request->seq();
    return CollectorResultCode::INVALID;
  }

  const uint64_t local_id = batch_response->local_id();
  AddPkg(local_id, std::move(batch_response));

  auto req = std::make_unique<Request>(*request);
  bool done = false;
  {
    std::unique_lock<std::mutex> lk(mutex_[local_id % 1000]);
    const int sender = request->sender_id();
    if (client_receive_senders_[local_id % 1000][local_id].insert(sender).second) {
      client_receive_[local_id % 1000][local_id].push_back(std::move(req));
    }
    // Zyzzyva client fast path: 3f+1 matching order-replies, one per replica.
    if (static_cast<int>(
            client_receive_senders_[local_id % 1000][local_id].size()) >=
        total_replicas_) {
      done = true;
    }
  }
  if (done) {
    Notify(local_id);
  }

  return 0;
}

bool ZZYPerformanceManager::SendTimeout(int local_id) {
  Proposal timeout_proposal;
  std::vector<std::unique_ptr<Request>> selected_prepares;
  int view = 1;
  std::string hash;
  int64_t seq = 0;

  {
    std::unique_lock<std::mutex> lk(mutex_[local_id % 1000]);
    auto& prepares = client_receive_[local_id % 1000][local_id];
    if (static_cast<int>(prepares.size()) < 2 * f_ + 1) {
      LOG(ERROR) << "not enough prepares for commit certificate, local_id:"
                 << local_id << " size:" << prepares.size();
      return false;
    }

    for (int i = 0; i < 2 * f_ + 1; ++i) {
      selected_prepares.push_back(std::make_unique<Request>(*prepares[i]));
    }

    hash = selected_prepares.front()->hash();
    seq = selected_prepares.front()->seq();
    if (selected_prepares.front()->current_view() > 0) {
      view = selected_prepares.front()->current_view();
    }
  }

  for (const auto& req : selected_prepares) {
    auto* qc = timeout_proposal.mutable_qc()->add_qc();
    const int qc_view = req->current_view() > 0 ? req->current_view() : view;
    qc->set_data_hash(
        BuildPrepareDigest(qc_view, req->seq(), req->hash(), req->sender_id()));
    *qc->mutable_sign() = req->data_signature();
    qc->set_seq(req->seq());
    qc->set_view(qc_view);
    qc->set_replica(req->sender_id());
  }

  timeout_proposal.set_hash(hash);
  timeout_proposal.set_seq(seq);
  timeout_proposal.set_proposer(id_);
  timeout_proposal.set_local_id(local_id);
  timeout_proposal.set_view(view);

  Broadcast(MessageType::Commit, timeout_proposal);
  return true;
}

}  // namespace zzy
}  // namespace resdb
