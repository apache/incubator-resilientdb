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

#include "platform/consensus/ordering/tendermint/commitment.h"

#include <glog/logging.h>
#include <unistd.h>

#include "common/utils/utils.h"

namespace resdb {
namespace tendermint {

const int TIMER_TIMEOUT_MS = 60000000;
class MonitorTimer {
 public:
  MonitorTimer(
      std::string name, int timeout_ms, int64_t h, int r,
      TendermintRequest::Type type,
      std::function<void(int64_t, int r, TendermintRequest::Type)> callback)
      : name_(name),
        timeout_ms_(timeout_ms),
        h_(h),
        r_(r),
        type_(type),
        callback_(callback),
        stop_(false) {
    thread_ = std::thread(&MonitorTimer::Process, this);
  }

  ~MonitorTimer() {
    Terminate();
    if (thread_.joinable()) {
      thread_.join();
    }
  }

  void Terminate() {
    stop_ = true;
    std::lock_guard<std::mutex> lk(mutex_);
    cv_.notify_all();
  }

  void Process() {
    std::unique_lock<std::mutex> lk(mutex_);
    cv_.wait_for(lk, std::chrono::microseconds(timeout_ms_),
                 [&] { return stop_ == true; });
    if (stop_) {
      return;
    }
    callback_(h_, r_, type_);
  }

 private:
  std::string name_;
  int timeout_ms_;
  int64_t h_;
  int r_;
  TendermintRequest::Type type_;
  std::function<void(int64_t, int, TendermintRequest::Type)> callback_;
  std::atomic<bool> stop_;
  std::thread thread_;
  std::condition_variable cv_;
  std::mutex mutex_;
};

Commitment::Commitment(const ResDBConfig& config,
                       MessageManager* message_manager,
                       ReplicaCommunicator* replica_client,
                       SignatureVerifier* verifier)
    : config_(config),
      message_manager_(message_manager),
      stop_(false),
      replica_client_(replica_client),
      verifier_(verifier),
      current_height_(1) {
  if (config_.GetPublicKeyCertificateInfo()
          .public_key()
          .public_key_info()
          .type() == CertificateKeyInfo::CLIENT) {
    return;
  }

  id_ = config_.GetSelfInfo().id();
  global_stats_ = Stats::GetGlobalStats();
  Reset();
  executed_thread_ = std::thread(&Commitment::PostProcessExecutedMsg, this);
  // only run once to trigger a new request.
  round_thread_ = std::thread(&Commitment::StartNewRound, this);
}

Commitment::~Commitment() {
  stop_ = true;
  if (executed_thread_.joinable()) {
    executed_thread_.join();
  }
  if (round_thread_.joinable()) {
    round_thread_.join();
  }
}

void Commitment::Init() { Reset(); }

void Commitment::Reset() {
  current_round_ = 0;
  current_step_ = TendermintRequest::TYPE_PROPOSAL;
  valid_round_ = -1;
}

void Commitment::SetNodeId(int32_t id) { id_ = id; }

int32_t Commitment::PrimaryId(int64_t view_num) {
  if (view_num <= 0) {
    view_num = 1;
  }
  view_num--;
  return (view_num % config_.GetReplicaInfos().size()) + 1;
}

std::unique_ptr<Request> Commitment::NewRequest(
    const TendermintRequest& request, TendermintRequest::Type type) {
  TendermintRequest new_request(request);
  new_request.set_type(type);
  new_request.set_sender_id(id_);
  auto ret = std::make_unique<Request>();
  new_request.SerializeToString(ret->mutable_data());
  return ret;
}

class Timer {
 public:
  Timer(std::string name) {
    start_ = GetCurrentTime();
    name_ = name;
  }
  ~Timer() {
    LOG(ERROR) << name_ << " run time:" << (GetCurrentTime() - start_);
  }

 private:
  uint64_t start_;
  std::string name_;
};

// 1. Client sends requests to the primary. Primary broadcasts a prepare
// message.
// 2. each replica responses a prepare vote.
// 3. Primary receives 2f+1 votes for prepare, sends out a pre-commit message.
int Commitment::Process(std::unique_ptr<TendermintRequest> request) {
  Timer timer(TendermintRequest_Type_Name(request->type()));
  switch (request->type()) {
    case TendermintRequest::TYPE_NEWREQUEST:
      return ProcessNewRequest(std::move(request));
    case TendermintRequest::TYPE_PROPOSAL:
    case TendermintRequest::TYPE_PREVOTE:
    case TendermintRequest::TYPE_PRECOMMIT:
      return ProcessRequest(std::move(request));
    default:
      return -2;
  }
}

int Commitment::ProcessNewRequest(std::unique_ptr<TendermintRequest> request) {
  request->set_hash(SignatureVerifier::CalculateHash(request->data()));
  global_stats_->IncClientRequest();
  request_list_.Push(std::move(request));
  return 0;
}

std::unique_ptr<TendermintRequest> Commitment::GetClientRequest() {
  while (!stop_) {
    auto request = request_list_.Pop(config_.ClientBatchWaitTimeMS());
    if (request == nullptr) {
      continue;
    }
    return request;
  }
  return nullptr;
}

void Commitment::AddTimer(const std::string& name,
                          TendermintRequest::Type next_type) {
  LOG(ERROR) << " set timeout:" << name;
  monitor_timers_.push_back(std::make_unique<MonitorTimer>(
      name, TIMER_TIMEOUT_MS, current_height_, current_round_, next_type,
      [&](int64_t h, int r, TendermintRequest::Type type) {
        LOG(ERROR) << " time out height:" << h << " round:" << r;
        std::unique_lock<std::mutex> lk(mutex_);
        if (h != current_height_ || r != current_round_) {
          return;
        }
        current_step_ = next_type;

        TendermintRequest precommit_request;
        precommit_request.set_round(r);
        precommit_request.set_height(h);
        LOG(ERROR) << " time out send:" << TendermintRequest_Type_Name(type);
        std::unique_ptr<Request> new_request =
            NewRequest(precommit_request, type);
        replica_client_->BroadCast(*new_request);
      }));
}

void Commitment::StartTimer() {
  if (current_step_ == TendermintRequest::TYPE_PROPOSAL) {
    AddTimer("proposal", TendermintRequest::TYPE_PREVOTE);
  } else if (current_step_ == TendermintRequest::TYPE_PREVOTE) {
    AddTimer("prevote", TendermintRequest::TYPE_PRECOMMIT);
  } else if (current_step_ == TendermintRequest::TYPE_PRECOMMIT) {
    LOG(ERROR) << "add commit timeout";
    monitor_timers_.push_back(std::make_unique<MonitorTimer>(
        "precommit", TIMER_TIMEOUT_MS, current_height_, current_round_,
        TendermintRequest::TYPE_NONE,
        [&](int64_t h, int r, TendermintRequest::Type) {
          LOG(ERROR) << " commit timeout";
          std::unique_lock<std::mutex> lk(mutex_);
          if (h != current_height_ || r != current_round_) {
            LOG(ERROR) << "height:" << h << " r:" << r << " not match";
            return;
          }
          std::this_thread::sleep_for(
              std::chrono::microseconds(TIMER_TIMEOUT_MS / 2));
          current_round_++;
          StartNewRound();
        }));
  }
}

std::unique_ptr<TendermintRequest> Commitment::FindPropose(int64_t h, int r) {
  if (proposal_.find(std::make_pair(h, r)) == proposal_.end()) {
    return nullptr;
  }
  return std::make_unique<TendermintRequest>(*proposal_[std::make_pair(h, r)]);
}

int Commitment::FindPrevote(int64_t h, int r, const std::string& hash,
                            bool all) {
  if (all) {
    std::set<int> tot;
    for (const auto& it : sender_[std::make_pair(h, r)]) {
      if (it.first.first == TendermintRequest::TYPE_PREVOTE) {
        for (auto id : it.second) {
          tot.insert(id);
        }
      }
    }
    return tot.size();
  }
  return prevote_[std::make_pair(h, r)][hash].size();
}

int Commitment::FindPrecommit(int64_t h, int r, const std::string& hash,
                              bool all) {
  if (all) {
    std::set<int> tot;
    for (const auto& it : sender_[std::make_pair(h, r)]) {
      if (it.first.first == TendermintRequest::TYPE_PRECOMMIT) {
        for (auto id : it.second) {
          tot.insert(id);
        }
      }
    }
    return tot.size();
  }
  return precommit_[std::make_pair(h, r)][hash].size();
}

// ========= Start function ===============
int Commitment::StartNewRound() {
  current_step_ = TendermintRequest::TYPE_PROPOSAL;
  if (PrimaryId(current_round_) != id_) {
    LOG(ERROR) << "not current primary:" << current_round_;
    // Redirect
    StartTimer();
    return -2;
  }
  std::unique_ptr<TendermintRequest> client_request = nullptr;
  if (valid_request_ != nullptr) {
    client_request = std::make_unique<TendermintRequest>(*valid_request_);
    LOG(ERROR) << "get valid request:";
  } else {
    client_request = GetClientRequest();
    assert(client_request != nullptr);
  }

  client_request->set_round(current_round_);
  client_request->set_height(current_height_);
  client_request->set_valid_round(valid_round_);

  std::unique_ptr<Request> new_request =
      NewRequest(*client_request, TendermintRequest::TYPE_PROPOSAL);
  LOG(ERROR) << "start new round:" << current_height_
             << " round:" << client_request->round()
             << " valid round:" << client_request->valid_round();
  replica_client_->BroadCast(*new_request);
  return 0;
}

int Commitment::ProcessRequest(std::unique_ptr<TendermintRequest> request) {
  int64_t h = request->height();
  int r = request->round();
  int sender_id = request->sender_id();
  std::string hash = request->hash();
  // LOG(ERROR)<<"process
  // request:"<<TendermintRequest_Type_Name(request->type())<<" h:"<<h<<"
  // r:"<<r<<" from:"<<sender_id<<" current
  // step:"<<TendermintRequest_Type_Name(current_step_.load())<<" hash:"<<hash;
  std::unique_lock<std::mutex> lk(mutex_);
  auto ret =
      sender_[std::make_pair(h, r)][std::make_pair(request->type(), hash)]
          .insert(request->sender_id());
  if (!ret.second) {
    LOG(ERROR) << " type:" << TendermintRequest_Type_Name(request->type())
               << " re-sent from:" << sender_id;
    return -2;
  }
  if (h < current_height_) {
    LOG(ERROR) << " h:" << h << " is outdated:" << current_height_;
    return -2;
  }

  if (request->type() == TendermintRequest::TYPE_PROPOSAL) {
    proposal_[std::make_pair(h, r)] = std::move(request);
  } else if (request->type() == TendermintRequest::TYPE_PREVOTE) {
    prevote_[std::make_pair(h, r)][request->hash()].push_back(
        std::move(request));
  } else if (request->type() == TendermintRequest::TYPE_PRECOMMIT) {
    precommit_[std::make_pair(h, r)][request->hash()].push_back(
        std::move(request));
  }

  if (h != current_height_) {
    LOG(ERROR) << " hieght not match, current height:" << current_height_;
    return -2;
  }

  if (r > current_round_) {
    outdated_[std::make_pair(h, r)].insert(sender_id);
    if (outdated_[std::make_pair(h, r)].size() ==
        config_.GetMaxMaliciousReplicaNum() + 1) {
      LOG(ERROR) << " update r:" << r << " from:" << current_round_;
      current_round_ = r;
      StartNewRound();
      return 0;
    }
  }
  if (r != current_round_) {
    LOG(ERROR) << " round not match, current round:" << current_round_;
    return -2;
  }

  if (current_step_ == TendermintRequest::TYPE_PREVOTE &&
      FindPrevote(h, r, hash, true) == config_.GetMinDataReceiveNum()) {
    StartTimer();
  }

  else if (current_step_ == TendermintRequest::TYPE_PRECOMMIT &&
           FindPrecommit(h, r, hash, true) == config_.GetMinDataReceiveNum()) {
    StartTimer();
  }

  return CheckStatus(h, r, hash);
}

void Commitment::CheckNextComplete() {
  std::unique_ptr<TendermintRequest> propose_request =
      FindPropose(current_height_, current_round_);
  if (propose_request == nullptr) {
    LOG(ERROR) << " check proposal not find, h:" << current_height_
               << " r:" << current_round_;
    return;
  }
  CheckStatus(current_height_, current_round_, propose_request->hash());
  return;
}

int Commitment::CheckStatus(int64_t h, int r, const std::string& hash) {
  if (hash.empty()) {
    if (current_step_ == TendermintRequest::TYPE_PREVOTE) {
      if (FindPrevote(h, r, hash) == config_.GetMinDataReceiveNum()) {
        TendermintRequest precommit_request;
        precommit_request.set_round(r);
        precommit_request.set_height(h);

        current_step_ = TendermintRequest::TYPE_PRECOMMIT;
        std::unique_ptr<Request> new_request =
            NewRequest(precommit_request, TendermintRequest::TYPE_PRECOMMIT);
        replica_client_->BroadCast(*new_request);
      }
      return 0;
    }
    return 0;
  }

  std::unique_ptr<TendermintRequest> propose_request = FindPropose(h, r);
  if (propose_request == nullptr) {
    LOG(ERROR) << " proposal not find, h:" << h << " r:" << r;
    return -2;
  }
  int vr = propose_request->valid_round();
  if (current_step_ == TendermintRequest::TYPE_PROPOSAL) {
    // upon ⟨PROPOSAL, hp, roundp, v, −1⟩
    // upon ⟨PROPOSAL, hp, roundp, v, vr⟩ AND 2f + 1 ⟨PREVOTE, hp, vr, id(v)⟩
    if (vr == -1 ||
        FindPrevote(h, vr, hash) >= config_.GetMinDataReceiveNum()) {
      TendermintRequest prevote_request(*propose_request);
      prevote_request.clear_data();
      if (lock_request_ && lock_request_->hash() != hash) {
        LOG(ERROR) << " lock value not equal:" << hash;
        prevote_request.clear_hash();
      }

      current_step_ = TendermintRequest::TYPE_PREVOTE;
      std::unique_ptr<Request> new_request =
          NewRequest(prevote_request, TendermintRequest::TYPE_PREVOTE);
      replica_client_->BroadCast(*new_request);
      return 0;
    }
    LOG(ERROR) << " proposal step not enough";
    return -2;
  } else {
    if (FindPrecommit(h, r, hash) == config_.GetMinDataReceiveNum()) {
      LOG(ERROR) << " ========== commit :" << propose_request->height()
                 << " ==========";
      message_manager_->Commit(std::move(propose_request));
      current_height_++;
      Reset();
      CheckNextComplete();
      StartNewRound();
      return 0;
    }
    if (FindPrevote(h, r, hash) == config_.GetMinDataReceiveNum() &&
        valid_round_ != r) {
      if (current_step_ == TendermintRequest::TYPE_PREVOTE) {
        lock_request_ = std::make_unique<TendermintRequest>(*propose_request);
        lock_round_ = r;

        TendermintRequest precommit_request(*propose_request);
        precommit_request.clear_data();
        precommit_request.set_round(r);
        precommit_request.set_height(h);

        current_step_ = TendermintRequest::TYPE_PRECOMMIT;
        std::unique_ptr<Request> new_request =
            NewRequest(precommit_request, TendermintRequest::TYPE_PRECOMMIT);
        replica_client_->BroadCast(*new_request);
      }
      valid_request_ = std::make_unique<TendermintRequest>(*propose_request);
      valid_round_ = r;
      return 0;
    }
  }
  return 0;
}

// =========== private threads ===========================
// If the transaction is executed, send back to the proxy.
int Commitment::PostProcessExecutedMsg() {
  while (!stop_) {
    auto batch_resp = message_manager_->GetResponseMsg();
    if (batch_resp == nullptr) {
      continue;
    }
    Request request;
    request.set_type(Request::TYPE_RESPONSE);
    request.set_sender_id(config_.GetSelfInfo().id());
    request.set_proxy_id(batch_resp->proxy_id());
    batch_resp->SerializeToString(request.mutable_data());
    replica_client_->SendMessage(request, request.proxy_id());
  }
  return 0;
}

}  // namespace tendermint
}  // namespace resdb
