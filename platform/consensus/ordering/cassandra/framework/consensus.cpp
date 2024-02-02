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

#include "platform/consensus/ordering/cassandra/framework/consensus.h"

#include <glog/logging.h>
#include <unistd.h>

#include "common/utils/utils.h"

namespace resdb {
namespace cassandra {

Consensus::Consensus(const ResDBConfig& config,
                     std::unique_ptr<TransactionManager> executor)
    : common::Consensus(config, std::move(executor)){
  int total_replicas = config_.GetReplicaNum();
  int f = (total_replicas - 1) / 3;

  Init();

  int batch_size = 10;
  start_ = 0;

  if (config_.GetPublicKeyCertificateInfo()
          .public_key()
          .public_key_info()
          .type() != CertificateKeyInfo::CLIENT) {
    cassandra_ = std::make_unique<cassandra_recv::Cassandra>(
        config_.GetSelfInfo().id(), f,
                                   total_replicas, GetSignatureVerifier());

    cassandra_->SetBroadcastCallFunc(
        [&](int type, const google::protobuf::Message& msg) {
          return Broadcast(type, msg);
        });

    cassandra_->SetCommitFunc([&](const google::protobuf::Message& msg) {
      return CommitMsg(dynamic_cast<const Transaction&>(msg));
    });
  }
}

int Consensus::ProcessCustomConsensus(std::unique_ptr<Request> request) {
  // LOG(ERROR)<<"receive commit:"<<request->type()<<"
  // "<<Request_Type_Name(request->type());
  if (request->user_type() == MessageType::NewBlocks) {
    std::unique_ptr<Block> block = std::make_unique<Block>();
    if (!block->ParseFromString(request->data())) {
      assert(1 == 0);
      LOG(ERROR) << "parse proposal fail";
      return -1;
    }
    cassandra_->ReceiveBlock(std::move(block));
    return 0;
  } else if (request->user_type() == MessageType::CMD_BlockACK) {
    std::unique_ptr<BlockACK> block_ack = std::make_unique<BlockACK>();
    if (!block_ack->ParseFromString(request->data())) {
      LOG(ERROR) << "parse proposal fail";
      assert(1 == 0);
      return -1;
    }
    cassandra_->ReceiveBlockACK(std::move(block_ack));
    return 0;

  } else if (request->user_type() == MessageType::NewProposal) {
    // LOG(ERROR)<<"receive proposal:";
    std::unique_ptr<Proposal> proposal = std::make_unique<Proposal>();
    if (!proposal->ParseFromString(request->data())) {
      LOG(ERROR) << "parse proposal fail";
      assert(1 == 0);
      return -1;
    }
    if (!cassandra_->ReceiveProposal(std::move(proposal))) {
      return -1;
    }
    return 0;
  } else if (request->user_type() == MessageType::CMD_BlockQuery) {
    std::unique_ptr<BlockQuery> block = std::make_unique<BlockQuery>();
    if (!block->ParseFromString(request->data())) {
      assert(1 == 0);
      LOG(ERROR) << "parse proposal fail";
      return -1;
    }
    cassandra_->SendBlock(*block);
    return 0;
  } else if (request->user_type() == MessageType::CMD_ProposalQuery) {
    std::unique_ptr<ProposalQuery> query =
      std::make_unique<ProposalQuery>();
    if (!query->ParseFromString(request->data())) {
      assert(1 == 0);
      LOG(ERROR) << "parse proposal fail";
      return -1;
    }
    cassandra_->SendProposal(*query);
  } else if (request->user_type() ==
      MessageType::CMD_ProposalQueryResponse) {
    std::unique_ptr<ProposalQueryResp> resp =
      std::make_unique<ProposalQueryResp>();
    if (!resp->ParseFromString(request->data())) {
      assert(1 == 0);
      LOG(ERROR) << "parse proposal fail";
      return -1;
    }
    cassandra_->ReceiveProposalQueryResp(*resp);
  } 
  return 0;
}

int Consensus::ProcessNewTransaction(std::unique_ptr<Request> request) {
  std::unique_ptr<Transaction> txn = std::make_unique<Transaction>();
  txn->set_data(request->data());
  txn->set_hash(request->hash());
  txn->set_proxy_id(request->proxy_id());
  return cassandra_->ReceiveTransaction(std::move(txn));
}

int Consensus::CommitMsg(const google::protobuf::Message& msg) {
  return CommitMsgInternal(dynamic_cast<const Transaction&>(msg));
}

int Consensus::CommitMsgInternal(const Transaction& txn) {
  // LOG(ERROR)<<"commit txn:"<<txn.id()<<" proxy id:"<<txn.proxy_id()<<"
  // uid:"<<txn.uid();
  std::unique_ptr<Request> request = std::make_unique<Request>();
  request->set_queuing_time(txn.queuing_time());
  request->set_data(txn.data());
  request->set_seq(txn.id());
  request->set_uid(txn.uid());
  if (txn.proposer_id() == config_.GetSelfInfo().id()) {
    request->set_proxy_id(txn.proxy_id());
    // LOG(ERROR)<<"commit txn:"<<txn.id()<<" proxy id:"<<txn.proxy_id();
  }

  transaction_executor_->AddExecuteMessage(std::move(request));
  // transaction_executor_->AddExecuteMessage(std::move(request));
  return 0;
}


}  // namespace cassandra
}  // namespace resdb
