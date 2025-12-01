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

#include "platform/consensus/ordering/multipaxos/framework/consensus.h"

#include <glog/logging.h>
#include <unistd.h>

#include "common/utils/utils.h"

namespace resdb {
namespace multipaxos {

std::unique_ptr<MultiPaxosPerformanceManager> Consensus::GetPerformanceManager() {
        return config_.IsPerformanceRunning()
        ? std::make_unique<MultiPaxosPerformanceManager>(
          config_, GetBroadCastClient(), GetSignatureVerifier())
        : nullptr;
}

Consensus::Consensus(const ResDBConfig& config,
                     std::unique_ptr<TransactionManager> executor)
    : common::Consensus(config, std::move(executor)) {

  SetPerformanceManager(GetPerformanceManager());

  Init();
  //failure_mode_ = config.GetConfigData().failure_mode();

  int total_replicas = config_.GetReplicaNum();
  int f = (total_replicas - 1) / 2;

  if (config_.GetPublicKeyCertificateInfo()
          .public_key()
          .public_key_info()
          .type() != CertificateKeyInfo::CLIENT) {
    multipaxos_= std::make_unique<MultiPaxos>(config_.GetSelfInfo().id(), f, total_replicas, 
    config_.GetConfigData().block_size(), GetSignatureVerifier());
    InitProtocol(multipaxos_.get());
  }
}

int Consensus::ProcessCustomConsensus(std::unique_ptr<Request> request) {
  //LOG(ERROR)<<"recv request:"<<MessageType_Name(request->user_type());
  //int64_t current_time = GetCurrentTime();
  //LOG(ERROR)<<" failure more:"<<failure_mode_;
  if(failure_mode_ && config_.GetSelfInfo().id() == 2) {
    return 0;
  }

  if(request->user_type() == MessageType::Propose) {
    std::unique_ptr<Proposal> p = std::make_unique<Proposal>();
    if (!p->ParseFromString(request->data())) {
      LOG(ERROR) << "parse proposal fail";
      assert(1 == 0);
      return -1;
    }
    multipaxos_->ReceiveProposal(std::move(p));
  }
  else if(request->user_type() == MessageType::Redirect) {
    std::unique_ptr<Transaction> p = std::make_unique<Transaction>();
    if (!p->ParseFromString(request->data())) {
      LOG(ERROR) << "parse proposal fail";
      assert(1 == 0);
      return -1;
    }
    multipaxos_->ReceiveTransaction(std::move(p));

  }
  else if(request->user_type() == MessageType::Query) {
    std::unique_ptr<Proposal> p = std::make_unique<Proposal>();
    if (!p->ParseFromString(request->data())) {
      LOG(ERROR) << "parse proposal fail";
      assert(1 == 0);
      return -1;
    }
    //multipaxos_->ReceivePropose(std::move(p));
  }
  else if(request->user_type() == MessageType::Accept) {
    std::unique_ptr<Proposal> p = std::make_unique<Proposal>();
    if (!p->ParseFromString(request->data())) {
      LOG(ERROR) << "parse proposal fail";
      assert(1 == 0);
      return -1;
    }
    multipaxos_->ReceiveAccept(std::move(p));
  }
  else if(request->user_type() == MessageType::Learn) {
    std::unique_ptr<Proposal> p = std::make_unique<Proposal>();
    if (!p->ParseFromString(request->data())) {
      LOG(ERROR) << "parse proposal fail";
      assert(1 == 0);
      return -1;
    }
    multipaxos_->ReceiveLearn(std::move(p));
  }
  return 0;
}

int Consensus::ProcessNewTransaction(std::unique_ptr<Request> request) {
  std::unique_ptr<Transaction> txn = std::make_unique<Transaction>();
  txn->set_data(request->data());
  txn->set_hash(request->hash());
  txn->set_proxy_id(request->proxy_id());
  txn->set_user_seq(request->user_seq());
  return multipaxos_->ReceiveTransaction(std::move(txn));
}

int Consensus::CommitMsg(const google::protobuf::Message& msg) {
  return CommitMsgInternal(dynamic_cast<const Transaction&>(msg));
}

int Consensus::CommitMsgInternal(const Transaction& txn) {
  //LOG(ERROR)<<"commit txn:"<<txn.id()<<" proxy id:"<<txn.proxy_id();
  std::unique_ptr<Request> request = std::make_unique<Request>();
  request->set_data(txn.data());
  request->set_seq(txn.id());
  request->set_proxy_id(txn.proxy_id());
  transaction_executor_->AddExecuteMessage(std::move(request));
  return 0;
}


}  // namespace fairdag
}  // namespace resdb
