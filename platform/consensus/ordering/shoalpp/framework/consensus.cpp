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

#include "platform/consensus/ordering/shoalpp/framework/consensus.h"

#include <glog/logging.h>
#include <unistd.h>

#include "common/utils/utils.h"

namespace resdb {
namespace shoalpp {

ShoalPPConsensus::ShoalPPConsensus(
    const ResDBConfig& config,
    std::unique_ptr<TransactionManager> executor)
    : Consensus(config, std::move(executor)) {
  int total_replicas = config_.GetReplicaNum();
  int f = (total_replicas - 1) / 3;

  Init();

  if (config_.GetPublicKeyCertificateInfo()
          .public_key()
          .public_key_info()
          .type() != CertificateKeyInfo::CLIENT) {
    shoalpp_ = std::make_unique<ShoalPP>(config_.GetSelfInfo().id(), f,
                                          total_replicas,
                                          GetSignatureVerifier(), config_);

    shoalpp_->SetSingleCallFunc(
        [&](int type, const google::protobuf::Message& msg, int node_id) {
          return SendMsg(type, msg, node_id);
        });

    shoalpp_->SetBroadcastCallFunc(
        [&](int type, const google::protobuf::Message& msg) {
          return Broadcast(type, msg);
        });

    shoalpp_->SetCommitFunc([&](const google::protobuf::Message& msg) {
      return CommitMsg(dynamic_cast<const Transaction&>(msg));
    });
  }
  LOG(ERROR) << "ShoalPP init consensus done";
}

int ShoalPPConsensus::ProcessCustomConsensus(
    std::unique_ptr<Request> request) {
  if (request->user_type() == MessageType::NewBlock) {
    std::unique_ptr<Proposal> p = std::make_unique<Proposal>();
    if (!p->ParseFromString(request->data())) {
      LOG(ERROR) << "parse proposal fail";
      assert(1 == 0);
      return -1;
    }
    shoalpp_->ReceiveBlock(std::move(p));
  } else if (request->user_type() == MessageType::BlockACK) {
    std::unique_ptr<Metadata> metadata = std::make_unique<Metadata>();
    if (!metadata->ParseFromString(request->data())) {
      LOG(ERROR) << "parse proposal fail";
      assert(1 == 0);
      return -1;
    }
    shoalpp_->ReceiveBlockACK(std::move(metadata));
  } else if (request->user_type() == MessageType::Cert) {
    std::unique_ptr<Certificate> cert = std::make_unique<Certificate>();
    if (!cert->ParseFromString(request->data())) {
      LOG(ERROR) << "parse proposal fail";
      assert(1 == 0);
      return -1;
    }
    shoalpp_->ReceiveBlockCert(std::move(cert));
  }
  return 0;
}

int ShoalPPConsensus::ProcessNewTransaction(
    std::unique_ptr<Request> request) {
  std::unique_ptr<Transaction> txn = std::make_unique<Transaction>();
  txn->set_data(request->data());
  txn->set_hash(request->hash());
  txn->set_proxy_id(request->proxy_id());
  return shoalpp_->ReceiveTransaction(std::move(txn));
}

int ShoalPPConsensus::CommitMsg(const google::protobuf::Message& msg) {
  return CommitMsgInternal(dynamic_cast<const Transaction&>(msg));
}

int ShoalPPConsensus::CommitMsgInternal(const Transaction& txn) {
  std::unique_ptr<Request> request = std::make_unique<Request>();
  request->set_queuing_time(txn.queuing_time());
  request->set_data(txn.data());
  request->set_seq(txn.id());
  request->set_proxy_id(txn.proxy_id());
  transaction_executor_->AddExecuteMessage(std::move(request));
  return 0;
}

}  // namespace shoalpp
}  // namespace resdb
