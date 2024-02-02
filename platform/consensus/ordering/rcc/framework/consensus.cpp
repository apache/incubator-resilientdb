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

#include "platform/consensus/ordering/rcc/framework/consensus.h"

#include <glog/logging.h>
#include <unistd.h>

#include "common/utils/utils.h"

namespace resdb {
namespace rcc {

Consensus::Consensus(const ResDBConfig& config,
                     std::unique_ptr<TransactionManager> executor)
    : common::Consensus(config, std::move(executor)){

  int total_replicas = config_.GetReplicaNum();
  int f = (total_replicas - 1) / 3;

  Init();

  if (config_.GetPublicKeyCertificateInfo()
          .public_key()
          .public_key_info()
          .type() != CertificateKeyInfo::CLIENT) {
    rcc_ = std::make_unique<RCC>(config_.GetSelfInfo().id(), f,
                                   total_replicas, GetSignatureVerifier());

    rcc_->SetSingleCallFunc(
        [&](int type, const google::protobuf::Message& msg, int node_id) {
          return SendMsg(type, msg, node_id);
        });

    rcc_->SetBroadcastCallFunc(
        [&](int type, const google::protobuf::Message& msg) {
          return Broadcast(type, msg);
        });

    rcc_->SetCommitFunc([&](const google::protobuf::Message& msg) {
      return CommitMsg(dynamic_cast<const Transaction&>(msg));
    });
  }
}

int Consensus::ProcessCustomConsensus(std::unique_ptr<Request> request) {

  if (request->user_type() == MessageType::ConsensusMsg) {
    Proposal proposal;
    if (!proposal.ParseFromString(request->data())) {
      assert(1 == 0);
      LOG(ERROR) << "parse proposal fail";
      return -1;
    }
    if (!rcc_->ReceiveProposal(proposal)) {
      return -1;
    }
    return 0;
  }
  return 0;
}

int Consensus::ProcessNewTransaction(std::unique_ptr<Request> request) {
  std::unique_ptr<Transaction> txn = std::make_unique<Transaction>();
  txn->set_data(request->data());
  txn->set_hash(request->hash());
  txn->set_proxy_id(request->proxy_id());
  return rcc_->ReceiveTransaction(std::move(txn));
}


int Consensus::CommitMsg(const google::protobuf::Message& msg) {
  return CommitMsgInternal(dynamic_cast<const Transaction&>(msg));
}

int Consensus::CommitMsgInternal(const Transaction& txn) {
  // LOG(ERROR)<<"commit txn:"<<txn.id()<<" proxy id:"<<txn.proxy_id();
  std::unique_ptr<Request> request = std::make_unique<Request>();
  request->set_queuing_time(txn.queuing_time());
  request->set_data(txn.data());
  request->set_seq(txn.id());
  request->set_proxy_id(txn.proxy_id());
  transaction_executor_->AddExecuteMessage(std::move(request));
  return 0;
}

}  // namespace rcc
}  // namespace resdb
