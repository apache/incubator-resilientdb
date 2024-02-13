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

#include "platform/consensus/ordering/fairdag/framework/consensus.h"

#include <glog/logging.h>
#include <unistd.h>

#include "common/utils/utils.h"

namespace resdb {
namespace fairdag {

std::unique_ptr<FairDAGPerformanceManager> FairDAGConsensus::GetPerformanceManager() {
        return config_.IsPerformanceRunning()
        ? std::make_unique<FairDAGPerformanceManager>(
          config_, GetBroadCastClient(), GetSignatureVerifier())
        : nullptr;
}

FairDAGConsensus::FairDAGConsensus(const ResDBConfig& config,
                     std::unique_ptr<TransactionManager> executor)
    : Consensus(config, std::move(executor)) {

  SetPerformanceManager(GetPerformanceManager());

  Init();

  int total_replicas = config_.GetReplicaNum();
  int f = (total_replicas - 1) / 3;

  if (config_.GetPublicKeyCertificateInfo()
          .public_key()
          .public_key_info()
          .type() != CertificateKeyInfo::CLIENT) {
    fairdag_= std::make_unique<FairDAG>(config_.GetSelfInfo().id(), f, total_replicas, GetSignatureVerifier());

    fairdag_->SetSingleCallFunc(
        [&](int type, const google::protobuf::Message& msg, int node_id) {
          return SendMsg(type, msg, node_id);
        });

    fairdag_->SetBroadcastCallFunc(
        [&](int type, const google::protobuf::Message& msg) {
          return Broadcast(type, msg);
        });

    fairdag_->SetCommitFunc(
        [&](const google::protobuf::Message& msg) { 
          return CommitMsg(dynamic_cast<const Transaction& >(msg)); 
        });
  }
}

int FairDAGConsensus::ProcessCustomConsensus(std::unique_ptr<Request> request) {
  //LOG(ERROR)<<"recv request:"<<MessageType_Name(request->user_type());
  //int64_t current_time = GetCurrentTime();
  if(request->user_type() == MessageType::NewBlock) {
    std::unique_ptr<Proposal> p = std::make_unique<Proposal>();
    if (!p->ParseFromString(request->data())) {
      LOG(ERROR) << "parse proposal fail";
      assert(1 == 0);
      return -1;
    }
    fairdag_->ReceiveBlock(std::move(p));
  }
  else if (request->user_type() == MessageType::BlockACK) {
    std::unique_ptr<Metadata> metadata = std::make_unique<Metadata>();
    if (!metadata->ParseFromString(request->data())) {
      LOG(ERROR) << "parse proposal fail";
      assert(1 == 0);
      return -1;
    }
    fairdag_->ReceiveBlockACK(std::move(metadata));
  }
  else if (request->user_type() == MessageType::Cert) {
    std::unique_ptr<Certificate> cert = std::make_unique<Certificate>();
    if (!cert->ParseFromString(request->data())) {
      LOG(ERROR) << "parse proposal fail";
      assert(1 == 0);
      return -1;
    }
    fairdag_->ReceiveBlockCert(std::move(cert));
  }
  return 0;
}

int FairDAGConsensus::ProcessNewTransaction(std::unique_ptr<Request> request) {
  std::unique_ptr<Transaction> txn = std::make_unique<Transaction>();
  txn->set_data(request->data());
  txn->set_hash(request->hash());
  txn->set_proxy_id(request->proxy_id());
  txn->set_user_seq(request->user_seq());
  return fairdag_->ReceiveTransaction(std::move(txn));
}

int FairDAGConsensus::CommitMsg(const google::protobuf::Message& msg) {
  return CommitMsgInternal(dynamic_cast<const Transaction&>(msg));
}

int FairDAGConsensus::CommitMsgInternal(const Transaction& txn) {
  //LOG(ERROR)<<"commit txn:"<<txn.id()<<" proxy id:"<<txn.proxy_id()<<" queuing delay:"<<txn.queuing_time();
  std::unique_ptr<Request> request = std::make_unique<Request>();
  request->set_queuing_time(txn.queuing_time());
  request->set_data(txn.data());
  request->set_seq(txn.id());
  request->set_proxy_id(txn.proxy_id());
  transaction_executor_->AddExecuteMessage(std::move(request));
  return 0;
}


}  // namespace fairdag
}  // namespace resdb
