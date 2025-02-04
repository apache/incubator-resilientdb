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

#include "platform/consensus/ordering/pbft/commitment.h"

#include <glog/logging.h>
#include <unistd.h>

#include "common/utils/utils.h"
#include "platform/consensus/ordering/pbft/transaction_utils.h"

namespace resdb {

Commitment::Commitment(const ResDBConfig& config,
                       MessageManager* message_manager,
                       ReplicaCommunicator* replica_communicator,
                       SignatureVerifier* verifier)
    : config_(config),
      message_manager_(message_manager),
      stop_(false),
      replica_communicator_(replica_communicator),
      verifier_(verifier) {
  executed_thread_ = std::thread(&Commitment::PostProcessExecutedMsg, this);
  global_stats_ = Stats::GetGlobalStats();
}

Commitment::~Commitment() {
  stop_ = true;
  if (executed_thread_.joinable()) {
    executed_thread_.join();
  }
}

void Commitment::SetPreVerifyFunc(
    std::function<bool(const Request& request)> func) {
  pre_verify_func_ = func;
}

void Commitment::SetNeedCommitQC(bool need_qc) { need_qc_ = need_qc; }

// Handle the user request and send a pre-prepare message to others.
// TODO if not a primary, redicet to the primary replica.
int Commitment::ProcessNewRequest(std::unique_ptr<Context> context,
                                  std::unique_ptr<Request> user_request) {
  if (context == nullptr || context->signature.signature().empty()) {
    LOG(ERROR) << "user request doesn't contain signature, reject";
    return -2;
  }
  // LOG(ERROR)<<"new request:"<<user_request->proxy_id();

  if (config_.GetSelfInfo().id() != message_manager_->GetCurrentPrimary()) {
    LOG(ERROR) << "current node is not primary. primary:"
               << message_manager_->GetCurrentPrimary()
               << " seq:" << user_request->seq();
    return -2;
  }

  if (pre_verify_func_ && !pre_verify_func_(*user_request)) {
    LOG(ERROR) << " check by the user func fail";
    return -2;
  }

  global_stats_->IncClientRequest();
  auto seq = message_manager_->AssignNextSeq();
  if (!seq.ok()) {
    global_stats_->SeqFail();
    Request request;
    request.set_type(Request::TYPE_RESPONSE);
    request.set_sender_id(config_.GetSelfInfo().id());
    request.set_proxy_id(user_request->proxy_id());
    request.set_ret(-2);

    replica_communicator_->SendMessage(request, request.proxy_id());
    return -2;
  }

  user_request->set_type(Request::TYPE_PRE_PREPARE);
  user_request->set_current_view(message_manager_->GetCurrentView());
  user_request->set_seq(*seq);
  user_request->set_sender_id(config_.GetSelfInfo().id());
  replica_communicator_->BroadCast(*user_request);
  return 0;
}

// Receive the pre-prepare message from the primary.
// TODO check whether the sender is the primary.
int Commitment::ProcessProposeMsg(std::unique_ptr<Context> context,
                                  std::unique_ptr<Request> request) {
  if (context == nullptr || context->signature.signature().empty()) {
    LOG(ERROR) << "user request doesn't contain signature, reject";
    return -2;
  }
  if (request->is_recovery()) {
    if (request->seq() >= message_manager_->GetNextSeq()) {
      message_manager_->SetNextSeq(request->seq() + 1);
    }
    return message_manager_->AddConsensusMsg(context->signature,
                                             std::move(request));
  }

  if (request->sender_id() != message_manager_->GetCurrentPrimary()) {
    LOG(ERROR) << "the request is not from primary. sender:"
               << request->sender_id() << " seq:" << request->seq();
    return -2;
  }

  /*
    if (request->sender_id() != config_.GetSelfInfo().id()) {
      if (pre_verify_func_ && !pre_verify_func_(*request)) {
        LOG(ERROR) << " check by the user func fail";
        return -2;
      }
      BatchUserRequest batch_request;
      batch_request.ParseFromString(request->data());
      batch_request.clear_createtime();
      std::string data;
      batch_request.SerializeToString(&data);
      // check signatures
      bool valid =
          verifier_->VerifyMessage(request->data(), request->data_signature());
      if (!valid) {
        LOG(ERROR) << "request is not valid:"
                   << request->data_signature().DebugString();
        LOG(ERROR) << " msg:" << request->data().size();
        return -2;
      }
    }
    */

  global_stats_->IncPropose();
  std::unique_ptr<Request> prepare_request = resdb::NewRequest(
      Request::TYPE_PREPARE, *request, config_.GetSelfInfo().id());
  prepare_request->clear_data();

  // Add request to message_manager.
  // If it has received enough same requests(2f+1), broadcast the prepare
  // message.
  CollectorResultCode ret =
      message_manager_->AddConsensusMsg(context->signature, std::move(request));
  if (ret == CollectorResultCode::STATE_CHANGED) {
    replica_communicator_->BroadCast(*prepare_request);
  }
  return ret == CollectorResultCode::INVALID ? -2 : 0;
}

// If receive 2f+1 prepare message, broadcast a commit message.
int Commitment::ProcessPrepareMsg(std::unique_ptr<Context> context,
                                  std::unique_ptr<Request> request) {
  if (context == nullptr || context->signature.signature().empty()) {
    LOG(ERROR) << "user request doesn't contain signature, reject";
    return -2;
  }
  if (request->is_recovery()) {
    return message_manager_->AddConsensusMsg(context->signature,
                                             std::move(request));
  }
  global_stats_->IncPrepare();
  std::unique_ptr<Request> commit_request = resdb::NewRequest(
      Request::TYPE_COMMIT, *request, config_.GetSelfInfo().id());
  commit_request->mutable_data_signature()->Clear();
  // Add request to message_manager.
  // If it has received enough same requests(2f+1), broadcast the commit
  // message.
  CollectorResultCode ret =
      message_manager_->AddConsensusMsg(context->signature, std::move(request));
  if (ret == CollectorResultCode::STATE_CHANGED) {
    /*
      // If need qc, sign the data
      if (need_qc_ && verifier_) {
        auto signature_or = verifier_->SignMessage(commit_request->hash());
        if (!signature_or.ok()) {
          LOG(ERROR) << "Sign message fail";
          return -2;
        }
        *commit_request->mutable_data_signature() = *signature_or;
        // LOG(ERROR) << "sign hash"
        //           << commit_request->data_signature().DebugString();
      }
      */
    replica_communicator_->BroadCast(*commit_request);
  }
  return ret == CollectorResultCode::INVALID ? -2 : 0;
}

// If receive 2f+1 commit message, commit the request.
int Commitment::ProcessCommitMsg(std::unique_ptr<Context> context,
                                 std::unique_ptr<Request> request) {
  if (context == nullptr || context->signature.signature().empty()) {
    LOG(ERROR) << "user request doesn't contain signature, reject"
               << " context:" << (context == nullptr);
    return -2;
  }
  if (request->is_recovery()) {
    return message_manager_->AddConsensusMsg(context->signature,
                                             std::move(request));
  }
  global_stats_->IncCommit();
  // Add request to message_manager.
  // If it has received enough same requests(2f+1), message manager will
  // commit the request.
  CollectorResultCode ret =
      message_manager_->AddConsensusMsg(context->signature, std::move(request));
  return ret == CollectorResultCode::INVALID ? -2 : 0;
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
    // LOG(ERROR)<<"send back to proxy:"<<batch_resp->proxy_id();
    batch_resp->SerializeToString(request.mutable_data());
    replica_communicator_->SendMessage(request, request.proxy_id());
  }
  return 0;
}

}  // namespace resdb
