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

#include "platform/consensus/ordering/poe_mac/commitment.h"

#include <glog/logging.h>
#include <unistd.h>

#include "common/utils/utils.h"

namespace resdb {
namespace poe {

Commitment::Commitment(const ResDBConfig& config,
                       MessageManager* message_manager,
                       ReplicaCommunicator* replica_communicator,
                       SignatureVerifier* verifier)
    : config_(config),
      message_manager_(message_manager),
      stop_(false),
      replica_communicator_(replica_communicator),
      verifier_(verifier),
      current_seq_(1) {
  id_ = config_.GetSelfInfo().id();
  global_stats_ = Stats::GetGlobalStats();
  executed_thread_ = std::thread(&Commitment::PostProcessExecutedMsg, this);
}

Commitment::~Commitment() {
  stop_ = true;
  if (executed_thread_.joinable()) {
    executed_thread_.join();
  }
}

void Commitment::Init() { Reset(1); }

void Commitment::Reset(uint64_t start_seq) {
  // LOG(ERROR) << " reset seq:" << start_seq;
  current_seq_ = start_seq;
  // received_requests_.clear();
  // received_senders_.clear();
}

void Commitment::SetNodeId(int32_t id) { id_ = id; }

int32_t Commitment::PrimaryId(int64_t view_num) {
  view_num--;
  return (view_num % config_.GetReplicaInfos().size()) + 1;
}

std::string GetHash(const Request& request) {
  std::string hash_data = request.data();
  // hash_data += std::to_string(request.view());
  hash_data += std::to_string(request.seq());
  return SignatureVerifier::CalculateHash(hash_data);
}

std::unique_ptr<Request> Commitment::NewRequest(const Request& request,
                                                Request::Type type) {
  auto ret = std::make_unique<Request>(request);
  ret->set_sender_id(id_);
  return ret;
}

std::unique_ptr<Request> Commitment::NewRequest(const POERequest& request,
                                                POERequest::Type type) {
  auto ret = std::make_unique<Request>();
  ret->set_type(type);
  ret->set_sender_id(id_);
  request.SerializeToString(ret->mutable_data());
  return ret;
}

class Timer {
 public:
  Timer(std::string name) {
    start_ = GetCurrentTime();
    name_ = name;
  }
  ~Timer() {
    // LOG(ERROR) << name_ << " run time:" << (GetCurrentTime() - start_);
  }

 private:
  uint64_t start_;
  std::string name_;
};

int Commitment::Process(std::unique_ptr<POERequest> request) {
  Timer timer(POERequest_Type_Name(request->type()));
  // LOG(ERROR) << " get process type:" << Request_Type_Name(request->type())
  //           << " seq:" << request->seq() << " from:" << request->sender_id();
  switch (request->type()) {
    case POERequest::TYPE_NEWREQUEST:
      return ProcessNewRequest(std::move(request));
    case POERequest::TYPE_SUPPORT:
      return ProcessSupport(std::move(request));
    case POERequest::TYPE_CERTIFY:
      return ProcessCeritify(std::move(request));
    default:
      return -2;
  }
  return 0;
}

int Commitment::ProcessNewRequest(std::unique_ptr<POERequest> request) {
  global_stats_->IncClientRequest();
  StartNewView(std::move(request));
  return 0;
}

// ========= Start function ===============
void Commitment::StartNewView(std::unique_ptr<POERequest> user_request) {
  auto seq = message_manager_->AssignNextSeq();
  if (!seq.ok()) {
    global_stats_->SeqFail();
    Request request;
    request.set_type(Request::TYPE_RESPONSE);
    request.set_sender_id(config_.GetSelfInfo().id());
    request.set_proxy_id(user_request->proxy_id());
    request.set_ret(-2);

    replica_communicator_->SendMessage(request, request.proxy_id());
    return;
  }

  POERequest poe_request(*user_request);
  // poe_request.set_data(user_request->data());
  poe_request.set_type(POERequest::TYPE_SUPPORT);
  // poe_request.set_hash(user_request->hash());
  poe_request.set_sender_id(id_);
  // poe_request.set_proxy_id(user_request->proxy_id());
  poe_request.set_seq(*seq);
  poe_request.set_view(message_manager_->GetCurrentView());

  std::unique_ptr<Request> support_request =
      NewRequest(poe_request, POERequest::TYPE_SUPPORT);
  replica_communicator_->BroadCast(*support_request);
}

int Commitment::ProcessSupport(std::unique_ptr<POERequest> request) {
  global_stats_->IncPropose();
  uint64_t curent_view = message_manager_->GetCurrentView();

  if (PrimaryId(curent_view) != request->sender_id()) {
    LOG(ERROR) << "not send from primary:";
    return -2;
  }

  DataCollector::CollectorResultCode ret = message_manager_->AddConsensusMsg(
      POETransactionCollector::ConvertToRequest(*request));
  if (ret == DataCollector::CollectorResultCode::STATE_CHANGED) {
    POERequest poe_request(*request);
    poe_request.set_sender_id(id_);
    poe_request.set_type(POERequest::TYPE_CERTIFY);
    poe_request.clear_data();

    std::unique_ptr<Request> new_request =
        NewRequest(poe_request, POERequest::TYPE_CERTIFY);

    replica_communicator_->BroadCast(*new_request);
  }
  return ret == DataCollector::CollectorResultCode::INVALID ? -2 : 0;
}

int Commitment::ProcessCeritify(std::unique_ptr<POERequest> request) {
  global_stats_->IncCommit();
  DataCollector::CollectorResultCode ret = message_manager_->AddConsensusMsg(
      POETransactionCollector::ConvertToRequest(*request));
  return ret == DataCollector::CollectorResultCode::INVALID ? -2 : 0;
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
    request.set_seq(batch_resp->seq());
    request.set_type(Request::TYPE_RESPONSE);
    request.set_sender_id(config_.GetSelfInfo().id());
    request.set_current_view(batch_resp->current_view());
    request.set_proxy_id(batch_resp->proxy_id());
    batch_resp->SerializeToString(request.mutable_data());
    replica_communicator_->SendMessage(request, request.proxy_id());
  }
  return 0;
}

void Commitment::SendFailRepsonse(const Request& user_request) {
  Request request;
  request.set_type(Request::TYPE_RESPONSE);
  request.set_sender_id(config_.GetSelfInfo().id());
  request.set_proxy_id(user_request.proxy_id());
  request.set_ret(-2);
  replica_communicator_->SendMessage(request, request.proxy_id());
}

void Commitment::RedirectUserRequest(const Request& user_request) {
  Request request;
  request.set_seq(user_request.seq());
  request.set_sender_id(config_.GetSelfInfo().id());
  request.set_proxy_id(user_request.proxy_id());
  user_request.SerializeToString(request.mutable_data());
  replica_communicator_->SendMessage(
      request, PrimaryId(message_manager_->GetCurrentView()));
}

}  // namespace poe
}  // namespace resdb
