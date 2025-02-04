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
    : ConsensusManager(config),
      replica_communicator_(GetBroadCastClient()),
      performance_manager_(
          config_.IsPerformanceRunning()
              ? std::make_unique<PerformanceManager>(
                    config_, GetBroadCastClient(), GetSignatureVerifier())
              : nullptr),
      response_manager_(
          !config_.IsPerformanceRunning()
              ? std::make_unique<ResponseManager>(config_, GetBroadCastClient(),
                                                  GetSignatureVerifier())
              : nullptr),

      transaction_executor_(std::make_unique<TransactionExecutor>(
          config,
          [&](std::unique_ptr<Request> request,
              std::unique_ptr<BatchUserResponse> resp_msg) {
            ResponseMsg(*resp_msg);
          },
          nullptr, std::move(executor))) {
  LOG(INFO) << "is running is performance mode:"
            << config_.IsPerformanceRunning();
  global_stats_ = Stats::GetGlobalStats();

  memset(send_num_, 0, sizeof(send_num_));
  int total_replicas = config_.GetReplicaNum();
  int f = (total_replicas - 1) / 3;
  int batch_size = 10;
  receive_msg_ = 0;
  start_ = 0;

  if (config_.GetPublicKeyCertificateInfo()
          .public_key()
          .public_key_info()
          .type() != CertificateKeyInfo::CLIENT) {
    // cassandra_ = std::make_unique<basic::Cassandra>(
    // cassandra_ = std::make_unique<cassandra_mem::Cassandra>(
    // cassandra_ = std::make_unique<cassandra_fp::Cassandra>(
    /*
    cassandra_ = std::make_unique<cassandra_recv::Cassandra>(
        config_.GetSelfInfo().id(), batch_size, total_replicas, f,
        [&](int type, const google::protobuf::Message& msg, int node_id) {
          return SendMsg(type, msg, node_id);
        },
        [&](int type, const google::protobuf::Message& msg) {
          return Broadcast(type, msg);
        },
        [&](const Transaction& msg) { return Commit(msg); });
        */
    cassandra_ = std::make_unique<cassandra_recv::Cassandra>(
        config_.GetSelfInfo().id(), batch_size, total_replicas, f,
        [&](int type, const google::protobuf::Message& msg, int node_id) {
          return SendMsg(type, msg, node_id);
        },
        [&](int type, const google::protobuf::Message& msg) {
          return Broadcast(type, msg);
        },
        [&](const Transaction& msg) { return Commit(msg); },
        [&](const Transaction& msg) { return Prepare(msg); });
  }
}

void Consensus::SetupPerformanceDataFunc(std::function<std::string()> func) {
  performance_manager_->SetDataFunc(func);
}

void Consensus::SetCommunicator(ReplicaCommunicator* replica_communicator) {
  replica_communicator_ = replica_communicator;
}

int Consensus::Broadcast(int type, const google::protobuf::Message& msg) {
  Request request;
  msg.SerializeToString(request.mutable_data());
  request.set_type(Request::TYPE_CUSTOM_CONSENSUS);
  request.set_user_type(type);
  request.set_sender_id(config_.GetSelfInfo().id());

  replica_communicator_->BroadCast(request);
  return 0;
}

int Consensus::SendMsg(int type, const google::protobuf::Message& msg,
                       int node_id) {
  Request request;
  msg.SerializeToString(request.mutable_data());
  request.set_type(Request::TYPE_CUSTOM_CONSENSUS);
  request.set_user_type(type);
  request.set_sender_id(config_.GetSelfInfo().id());
  replica_communicator_->SendMessage(request, node_id);
  return 0;
}

std::vector<ReplicaInfo> Consensus::GetReplicas() {
  return config_.GetReplicaInfos();
}

int Consensus::Commit(const Transaction& txn) {
  // LOG(ERROR)<<"commit txn:"<<txn.id()<<" proxy id:"<<txn.proxy_id()<<"
  // uid:"<<txn.uid();
  std::unique_ptr<Request> request = std::make_unique<Request>();
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

int Consensus::Prepare(const Transaction& txn) {
  // LOG(ERROR)<<"prepare txn:"<<txn.id()<<" proxy id:"<<txn.proxy_id()<<"
  // uid:"<<txn.uid();
  std::unique_ptr<Request> request = std::make_unique<Request>();
  request->set_data(txn.data());
  request->set_uid(txn.uid());
  transaction_executor_->Prepare(std::move(request));
  // transaction_executor_->AddExecuteMessage(std::move(request));
  return 0;
}

// The implementation of PBFT.
int Consensus::ConsensusCommit(std::unique_ptr<Context> context,
                               std::unique_ptr<Request> request) {
  // LOG(ERROR)<<"receive commit:"<<request->type()<<"
  // "<<Request_Type_Name(request->type());
  receive_msg_++;
  switch (request->type()) {
    case Request::TYPE_CLIENT_REQUEST:
      if (config_.IsPerformanceRunning()) {
        return performance_manager_->StartEval();
      }
    case Request::TYPE_RESPONSE:
      if (config_.IsPerformanceRunning()) {
        return performance_manager_->ProcessResponseMsg(std::move(context),
                                                        std::move(request));
      }
    case Request::TYPE_NEW_TXNS: {
      std::unique_ptr<Transaction> txn = std::make_unique<Transaction>();
      txn->set_data(request->data());
      txn->set_hash(request->hash());
      txn->set_proxy_id(request->proxy_id());
      txn->set_proposer_id(config_.GetSelfInfo().id());
      if (cassandra_->ReceiveTransaction(std::move(txn))) {
        return 0;
      }
      return -1;
    }
    case Request::TYPE_CUSTOM_CONSENSUS: {
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

      } else if (request->user_type() == MessageType::Recovery) {
      } else {
        return 0;
      }
      return -1;
    }
  }
  return 0;
}

int Consensus::ResponseMsg(const BatchUserResponse& batch_resp) {
  // LOG(ERROR)<<"send response:"<<batch_resp.proxy_id();
  if (batch_resp.proxy_id() == 0) {
    return 0;
  }
  // LOG(ERROR)<<"send response:"<<batch_resp.proxy_id();
  Request request;
  request.set_seq(batch_resp.seq());
  request.set_type(Request::TYPE_RESPONSE);
  request.set_sender_id(config_.GetSelfInfo().id());
  request.set_proxy_id(batch_resp.proxy_id());
  batch_resp.SerializeToString(request.mutable_data());
  {
    std::unique_lock<std::mutex> lk(mutex_);
    send_num_[batch_resp.proxy_id()]++;
    if (send_num_[batch_resp.proxy_id()] % 10 != 0) {
      return 0;
    }
  }
  // LOG(ERROR)<<"send back to proxy:"<<send_num_;
  // LOG(ERROR)<<"send back to proxy:"<<batch_resp.proxy_id()
  //  <<" seq:"<<batch_resp.seq()
  //  <<" local id:"<<batch_resp.local_id()<<" data
  //  size:"<<request.data().size();
  replica_communicator_->SendMessage(request, request.proxy_id());
  return 0;
}

}  // namespace cassandra
}  // namespace resdb
