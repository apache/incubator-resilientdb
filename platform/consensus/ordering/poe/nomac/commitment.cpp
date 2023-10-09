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

#include "platform/consensus/ordering/poe/nomac/commitment.h"

#include <glog/logging.h>
#include <unistd.h>

#include "common/utils/utils.h"
#include "platform/consensus/ordering/poe/common/poe_utils.h"

namespace resdb {
namespace poe {

Commitment::Commitment(
    const ResDBConfig& config, MessageManager* message_manager,
    ReplicaCommunicator* replica_communicator, SignatureVerifier* verifier,
    std::function<int(int, const ::google::protobuf::Message& message, int)>
        client_call)
    : config_(config),
      message_manager_(message_manager),
      stop_(false),
      verifier_(verifier),
      client_call_(client_call) {
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

void Commitment::SetNodeId(int32_t id) { id_ = id; }

int32_t Commitment::PrimaryId(int64_t view_num) {
  view_num--;
  return (view_num % config_.GetReplicaInfos().size()) + 1;
}

POERequest::Type Commitment::VoteType(int type) {
  switch (type) {
    case POERequest::TYPE_PROPOSE: {
      return POERequest::TYPE_SUPPORT;
    }
    default:
      return POERequest::TYPE_NONE;
  }
}

std::string GetHash(const POERequest& request) {
  std::string hash_data = request.data();
  hash_data += std::to_string(request.current_view());
  hash_data += std::to_string(request.seq());
  return SignatureVerifier::CalculateHash(hash_data);
}

std::unique_ptr<Request> Commitment::NewRequest(const POERequest& request,
                                                POERequest::Type type) {
  POERequest new_request(request);
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
    // LOG(ERROR) << name_ << " run time:" << (GetCurrentTime() - start_);
  }

 private:
  uint64_t start_;
  std::string name_;
};

int Commitment::Process(int type, std::unique_ptr<POERequest> request) {
  Timer timer(POERequest_Type_Name(request->type()));
  // LOG(ERROR) << " get process type:" << POERequest_Type_Name(type)
  //          << " seq:" << request->seq() << " from:" << request->sender_id();
  switch (type) {
    case POERequest::TYPE_SUPPORT:
      return ProcessMessageOnPrimary(std::move(request));
    case POERequest::TYPE_PROPOSE:
    case POERequest::TYPE_CERTIFY:
      return ProcessMessageOnReplica(std::move(request));
    default:
      return -2;
  }
}

int Commitment::ProcessNewTransaction(std::unique_ptr<Request> request) {
  global_stats_->IncClientRequest();
  StartNewView(std::move(request));
  return 0;
}

// ========= Start function ===============
void Commitment::StartNewView(std::unique_ptr<Request> user_request) {
  // LOG(ERROR) << "start primary id:" << PrimaryId(current_view_) << " "
  //         << current_view_;
  if (PrimaryId(message_manager_->GetCurrentView()) != id_) {
    LOG(ERROR) << "not a primary, redirect to:"
               << message_manager_->GetCurrentView();
    RedirectUserRequest(*user_request);
    return;
  }

  if (message_manager_->IsLockCurrentView()) {
    LOG(ERROR) << " view is lock:" << message_manager_->GetCurrentView();
    SendFailRepsonse(*user_request);
    return;
  }

  auto seq = message_manager_->AssignNextSeq();
  if (!seq.ok()) {
    global_stats_->SeqFail();
    SendFailRepsonse(*user_request);
    return;
  }

  std::unique_ptr<POERequest> poe_request =
      ConvertToCustomRequest(*user_request);

  poe_request->set_type(POERequest::TYPE_PROPOSE);
  poe_request->set_seq(*seq);
  poe_request->set_current_view(message_manager_->GetCurrentView());
  poe_request->set_sender_id(id_);
  *poe_request->mutable_data_signature() = user_request->data_signature();
  client_call_(POERequest::TYPE_PROPOSE, *poe_request, -1);
}

bool Commitment::VerifyNodeSignagure(const POERequest& request) {
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

bool Commitment::VerifyTS(const POERequest& request) {
  if (verifier_ == nullptr) {
    return true;
  }
  if (request.qc().signatures_size() < config_.GetMinDataReceiveNum()) {
    LOG(ERROR) << "signature is not enough:" << request.qc().signatures_size()
               << " seq:" << request.seq();
    return false;
  }
  for (const auto& signature : request.qc().signatures()) {
    bool valid = verifier_->VerifyMessage(request.hash(), signature);
    if (!valid) {
      LOG(ERROR) << "TS is not valid:" << request.seq();
      return false;
    }
  }
  return true;
}

bool Commitment::VerifyTxn(const POERequest& request) {
  // Verify the client signature
  if (request.sender_id() != config_.GetSelfInfo().id()) {
    // check signatures
    bool valid =
        verifier_->VerifyMessage(request.data(), request.data_signature());
    if (!valid) {
      LOG(ERROR) << "request is not valid:"
                 << request.data_signature().DebugString();
      LOG(ERROR) << " msg:" << request.data().size();
      return false;
    }
  }
  return true;
}

int Commitment::ProcessMessageOnPrimary(std::unique_ptr<POERequest> request) {
  // POERequest::Type type = (POERequest::Type)request->type();
  // LOG(INFO) << "primary get type:" << POERequest_Type_Name(type)
  //          << " view:" << request->current_view() << " seq:" <<
  //          request->seq()
  //          << " from:" << request->sender_id()
  //          << " request:" << request->DebugString();
  if (!VerifyNodeSignagure(*request)) {
    LOG(ERROR) << "signature invalid";
    return -2;
  }

  std::unique_ptr<Request> req = ConvertToRequest(*request);

  if (request->has_data_signature()) {
    *req->mutable_data_signature() = request->data_signature();
  }

  CollectorResultCode ret = message_manager_->AddConsensusMsg(std::move(req));
  if (ret == CollectorResultCode::STATE_CHANGED) {
    POERequest poe_request(*request);
    *poe_request.mutable_qc() = message_manager_->GetQC(request->seq());
    assert(poe_request.qc().signatures_size() > 0);

    poe_request.set_sender_id(id_);
    poe_request.set_type(POERequest::TYPE_CERTIFY);
    client_call_(POERequest::TYPE_CERTIFY, poe_request, -1);
    message_manager_->Clear(poe_request.seq());
  }
  return 0;
}

int Commitment::ProcessMessageOnReplica(std::unique_ptr<POERequest> request) {
  // LOG(ERROR) << "Replica receive type:" <<
  // POERequest_Type_Name(request->type())
  //           << " from:" << request->sender_id() << " seq:" << request->seq();
  if (request->type() == POERequest::TYPE_PROPOSE) {
    global_stats_->IncPropose();
  }
  if (request->type() == POERequest::TYPE_CERTIFY) {
    global_stats_->IncCommit();
  }
  if (request->type() == POERequest::TYPE_PROPOSE) {
    uint64_t curent_view = message_manager_->GetCurrentView();
    if (request->current_view() != curent_view) {
      LOG(ERROR) << "current view is not the same:" << request->current_view()
                 << " current view:" << curent_view;
      return -2;
    }
    if (PrimaryId(curent_view) != request->sender_id()) {
      LOG(ERROR) << "not send from primary:";
      return -2;
    }
    // check if has been received.
    if (message_manager_->IsCommitted(request->seq())) {
      LOG(ERROR) << "current seq is larger:" << request->seq();
      return -2;
    }
    // Verify the client signature
    if (!VerifyTxn(*request)) {
      return -2;
    }
  } else {
    if (!VerifyTS(*request)) {
      LOG(ERROR) << " ts not invlid"
                 << " request type:" << POERequest_Type_Name(request->type())
                 << " seq:" << request->seq();
      return -2;
    }
  }

  // LOG(INFO) << "Replica receive type:" <<
  // POERequest_Type_Name(request->type())
  //          << " view:" << request->view() << " seq:" << request->seq()
  //          << " from:" << request->sender_id();
  if (request->type() == POERequest::TYPE_CERTIFY) {
    // execute and send new view
    message_manager_->Commit(std::move(request));
  } else {
    POERequest poe_request(*request);
    int sender = poe_request.sender_id();
    poe_request.set_hash(GetHash(*request));
    poe_request.clear_data();
    if (verifier_) {
      auto signature_or = verifier_->SignMessage(poe_request.hash());
      if (!signature_or.ok()) {
        LOG(ERROR) << "Sign message fail";
        return -2;
      }
      *poe_request.mutable_data_signature() = *signature_or;
    }
    request->set_hash(poe_request.hash());
    message_manager_->SavePreparedData(std::move(request));

    // LOG(INFO) << "send type:"
    //           << POERequest_Type_Name(VoteType(poe_request.type()))
    //          << " to:" << poe_request.sender_id() << " view"
    //         << poe_request.current_view() << " seq:" << poe_request.seq();
    poe_request.set_type(VoteType(poe_request.type()));
    poe_request.set_sender_id(id_);
    client_call_(poe_request.type(), poe_request, sender);
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
    request.set_seq(batch_resp->seq());
    request.set_type(Request::TYPE_RESPONSE);
    request.set_sender_id(config_.GetSelfInfo().id());
    request.set_current_view(batch_resp->current_view());
    request.set_proxy_id(batch_resp->proxy_id());
    batch_resp->SerializeToString(request.mutable_data());
    client_call_(0, request, request.proxy_id());
  }
  return 0;
}

void Commitment::SendFailRepsonse(const Request& user_request) {
  Request request;
  request.set_type(Request::TYPE_RESPONSE);
  request.set_sender_id(config_.GetSelfInfo().id());
  request.set_proxy_id(user_request.proxy_id());
  request.set_ret(-2);
  client_call_(0, request, request.proxy_id());
}

void Commitment::RedirectUserRequest(const Request& user_request) {
  Request request;
  request.set_seq(user_request.seq());
  request.set_sender_id(config_.GetSelfInfo().id());
  request.set_proxy_id(user_request.proxy_id());
  user_request.SerializeToString(request.mutable_data());
  client_call_(0, request, PrimaryId(message_manager_->GetCurrentView()));
}

}  // namespace poe
}  // namespace resdb
