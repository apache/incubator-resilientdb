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

#include "platform/consensus/ordering/zzy/framework/consensus.h"

#include <glog/logging.h>
#include <unistd.h>

#include "common/utils/utils.h"

namespace resdb {
namespace zzy {

std::unique_ptr<ZZYPerformanceManager> Consensus::GetPerformanceManager() {
        return config_.IsPerformanceRunning()
        ? std::make_unique<ZZYPerformanceManager>(
          config_, GetBroadCastClient(), GetSignatureVerifier())
        : nullptr;
}


Consensus::Consensus(const ResDBConfig& config,
    std::unique_ptr<TransactionManager> executor)
  : common::Consensus(config, std::move(executor)){
    int total_replicas = config_.GetReplicaNum();
    int f = (total_replicas - 1) / 3;

    if (config_.GetPublicKeyCertificateInfo()
        .public_key()
        .public_key_info()
        .type() == CertificateKeyInfo::CLIENT) {
      SetPerformanceManager(GetPerformanceManager());
    }

    Init();

    start_ = 0;

    if (config_.GetPublicKeyCertificateInfo()
        .public_key()
        .public_key_info()
        .type() != CertificateKeyInfo::CLIENT) {
      zzy_ = std::make_unique<ZZY>(
          config_, config_.GetSelfInfo().id(), f,
          total_replicas, GetSignatureVerifier());
      InitProtocol(zzy_.get());
      zzy_->SetFailFunc([&](const Transaction& txn) {
          SendFail(txn.proxy_id(), txn.hash());
        });
    }
  }

int Consensus::ProcessCustomConsensus(std::unique_ptr<Request> request) {
  if (request->user_type() == MessageType::Propose) {
    std::unique_ptr<Transaction> txn = std::make_unique<Transaction>();
    if (!txn->ParseFromString(request->data())) {
      assert(1 == 0);
      LOG(ERROR) << "parse proposal fail";
      return -1;
    }
    zzy_->ReceivePropose(std::move(txn));
    return 0;
  }
  else if (request->user_type() == MessageType::CommitACK) {
    std::unique_ptr<Proposal> proposal = std::make_unique<Proposal>();
    if (!proposal->ParseFromString(request->data())) {
      LOG(ERROR) << "parse proposal fail";
      assert(1 == 0);
      return -1;
    }
    dynamic_cast<ZZYPerformanceManager*>(performance_manager_.get())->ReceiveCommitACK(std::move(proposal));
  }
  else if (request->user_type() == MessageType::Commit) {
    std::unique_ptr<Proposal> proposal = std::make_unique<Proposal>();
    if (!proposal->ParseFromString(request->data())) {
      LOG(ERROR) << "parse proposal fail";
      assert(1 == 0);
      return -1;
    }
    zzy_->ReceiveCommit(std::move(proposal));
  }
  return 0;
}

int Consensus::ProcessNewTransaction(std::unique_ptr<Request> request) {

  
  std::unique_ptr<Transaction> txn = std::make_unique<Transaction>();
  txn->set_data(request->data());
  txn->set_hash(request->hash());
  txn->set_proxy_id(request->proxy_id());
  txn->set_uid(request->uid());
  int proxy_id = txn->proxy_id();
  std::string hash = txn->hash();
  //LOG(ERROR)<<"receive txn";
  bool ret = zzy_->ReceiveTransaction(std::move(txn));
  if(!ret){
    SendFail(proxy_id, hash);
  }
  return ret;
}

int Consensus::CommitMsg(const google::protobuf::Message& msg) {
  return CommitMsgInternal(dynamic_cast<const Transaction&>(msg));
}

int Consensus::CommitMsgInternal(const Transaction& txn) {
  //LOG(ERROR)<<"commit txn:"<<txn.seq()<<" proxy id:"<<txn.proxy_id()<<" uid:"<<txn.uid();
  std::unique_ptr<Request> request = std::make_unique<Request>();
  request->set_data(txn.data());
  request->set_seq(txn.seq());
  request->set_uid(txn.uid());
  request->set_proxy_id(txn.proxy_id());
  request->set_hash(txn.hash());

  transaction_executor_->Commit(std::move(request));
  return 0;
}

int Consensus::ResponseMsg(const Request &request, const BatchUserResponse& batch_resp){
  if(config_.GetSelfInfo().id() == 2 && batch_resp.local_id() > 10) {
  //  return true;
  }
    Request resp_request;
    resp_request.set_seq(batch_resp.seq());
    resp_request.set_type(Request::TYPE_RESPONSE);
    resp_request.set_sender_id(config_.GetSelfInfo().id());
    resp_request.set_proxy_id(batch_resp.proxy_id());
    resp_request.set_hash(request.hash());

    batch_resp.SerializeToString(resp_request.mutable_data());


    if (verifier_) {
       resp_request.set_data_hash(SignatureVerifier::CalculateHash(request.data()));
      auto signature_or = verifier_->SignMessage(resp_request.data_hash());
      if (!signature_or.ok()) {
        LOG(ERROR) << "Sign message fail";
        return -2;
      }
      *resp_request.mutable_data_signature() = *signature_or;
      //LOG(ERROR)<<" sign:";
    }

    //LOG(ERROR)<<" send back to proxy:"<<request.proxy_id();
    //global_stats_->AddResponseDelay(GetCurrentTime()- batch_resp.createtime());
    replica_communicator_->SendMessage(resp_request, resp_request.proxy_id());
  return 0;
}


}  // namespace zzy
}  // namespace resdb
