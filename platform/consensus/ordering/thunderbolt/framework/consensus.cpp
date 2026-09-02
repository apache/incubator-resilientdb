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

#include "platform/consensus/ordering/thunderbolt/framework/consensus.h"

#include <glog/logging.h>
#include <unistd.h>

#include "common/utils/utils.h"

namespace resdb {
namespace thunderbolt {

ThunderboltConsensus::ThunderboltConsensus(const ResDBConfig& config,
                             std::unique_ptr<TransactionManager> executor)
    : Consensus(config, std::move(executor)) {
  int total_replicas = config_.GetReplicaNum();
  int f = (total_replicas - 1) / 3;
  f_ = f;
  id_ = config_.GetSelfInfo().id();

  if (config_.GetPublicKeyCertificateInfo()
          .public_key()
          .public_key_info()
          .type() != CertificateKeyInfo::CLIENT) {
    thunderbolt_ = std::make_unique<Thunderbolt>(config_.GetSelfInfo().id(), f,
                                   total_replicas, GetSignatureVerifier());

    thunderbolt_->SetSingleCallFunc(
        [&](int type, const google::protobuf::Message& msg, int node_id) {
          return SendMsg(type, msg, node_id);
        });

    thunderbolt_->SetBroadcastCallFunc(
        [&](int type, const google::protobuf::Message& msg) {
          return Broadcast(type, msg);
        });

    thunderbolt_->SetCommitFunc([&](const google::protobuf::Message& msg) {
      return CommitMsg(dynamic_cast<const Transaction&>(msg));
    });
  }
  preprocess_thread_ = std::thread(&ThunderboltConsensus::AsyncPreprocess, this);
  transaction_executor_->SetUserFunc(
      [&](int proposer) { StopCallBack(proposer); });
}

void ThunderboltConsensus::StopCallBack(int proposer) {
  int dag_id = proposer >> 16;
  // int current_id = thunderbolt_->DagID();
  proposer = proposer & ((1 << 16) - 1);
  assert(proposer > 0);
  stop_proposer_.insert(proposer);
  // LOG(ERROR)<<" done stop from:"<<proposer<<"
  // size:"<<stop_proposer_.size()<<" dag:"<<dag_id<<" current
  // dag:"<<current_id<<" "<<thunderbolt_->DagID()<<" f:"<<f_;
  assert(dag_id == thunderbolt_->DagID());
  if (stop_proposer_.size() >= 2 * f_ + 1) {
    thunderbolt_->StopDone();
    stop_proposer_.clear();
  }
}

int ThunderboltConsensus::ProcessCustomConsensus(std::unique_ptr<Request> request) {
  if (id_ <= 0) {
    return true;
  }
  // LOG(ERROR)<<"recv request:"<<MessageType_Name(request->user_type());
  // int64_t current_time = GetCurrentTime();
  if (request->user_type() == MessageType::NewBlock) {
    std::unique_ptr<Proposal> p = std::make_unique<Proposal>();
    if (!p->ParseFromString(request->data())) {
      LOG(ERROR) << "parse proposal fail";
      assert(1 == 0);
      return -1;
    }
    thunderbolt_->ReceiveBlock(std::move(p));
  } else if (request->user_type() == MessageType::BlockACK) {
    std::unique_ptr<Metadata> metadata = std::make_unique<Metadata>();
    if (!metadata->ParseFromString(request->data())) {
      LOG(ERROR) << "parse proposal fail";
      assert(1 == 0);
      return -1;
    }
    thunderbolt_->ReceiveBlockACK(std::move(metadata));
  } else if (request->user_type() == MessageType::Cert) {
    std::unique_ptr<Certificate> cert = std::make_unique<Certificate>();
    if (!cert->ParseFromString(request->data())) {
      LOG(ERROR) << "parse proposal fail";
      assert(1 == 0);
      return -1;
    }
    thunderbolt_->ReceiveBlockCert(std::move(cert));
  }
  return 0;
}

void ThunderboltConsensus::AsyncPreprocess() {
  while (!IsStop()) {
    auto request = pre_q_.Pop();
    if (request == nullptr) {
      continue;
    }

    bool ret = thunderbolt_->WaitForNext(request->belong_shards_size() > 0);

    if (request->belong_shards_size() == 0 && ret) {
      preprocess_func_(request.get());
    }
    {
      std::unique_ptr<Transaction> txn = std::make_unique<Transaction>();
      txn->set_data(request->data());
      txn->set_hash(request->hash());
      txn->set_proxy_id(request->proxy_id());
      *txn->mutable_belong_shards() = request->belong_shards();
      txn->set_shard_id(request->shard_id());
      // LOG(ERROR)<<" generate txn group size:"<<txn->belong_shards_size()<<"
      // request group size:"<<request->belong_shards_size()<<" wait res:"<<ret;
      thunderbolt_->ReceiveTransaction(std::move(txn));
    }
  }
}

void ThunderboltConsensus::SetVerifiyFunc(std::function<bool(const Request&)> func) {
  verify_func_ = func;
}

bool ThunderboltConsensus::VerifyTransaction(const Transaction& txn) {
  if (!verify_func_) {
    return true;
  }
  Request request;
  request.set_data(txn.data());
  return verify_func_(request);
}

void ThunderboltConsensus::SetPreprocessFunc(std::function<int(Request*)> func) {
  preprocess_func_ = func;
}

void ThunderboltConsensus::Preprocess(std::unique_ptr<Request> request) {
  pre_q_.Push(std::move(request));
}

int ThunderboltConsensus::ProcessNewTransaction(std::unique_ptr<Request> request) {
  // LOG(ERROR)<<" process new txn";
  if (id_ <= 0) {
    return true;
  }
  if (preprocess_func_) {
    Preprocess(std::move(request));
    return 0;
  } else {
    std::unique_ptr<Transaction> txn = std::make_unique<Transaction>();
    txn->set_data(request->data());
    txn->set_hash(request->hash());
    txn->set_proxy_id(request->proxy_id());
    txn->set_shard_id(request->shard_id());
    if (request->belong_shards().empty()) {
      request->add_belong_shards(request->shard_id());
    }
    *txn->mutable_belong_shards() = request->belong_shards();
    return thunderbolt_->ReceiveTransaction(std::move(txn));
  }
}

int ThunderboltConsensus::CommitMsg(const google::protobuf::Message& msg) {
  return CommitMsgInternal(dynamic_cast<const Transaction&>(msg));
}

int ThunderboltConsensus::CommitMsgInternal(const Transaction& txn) {
  // LOG(ERROR)<<"commit txn:"<<txn.id()<<" proxy id:"<<txn.proxy_id()<<"
  // skip:"<<txn.skip();
  std::unique_ptr<Request> request = std::make_unique<Request>();
  request->set_data(txn.data());
  request->set_seq(txn.id());
  request->set_proxy_id(txn.proxy_id());
  request->set_create_time(txn.create_time());
  request->set_commit_time(GetCurrentTime());
  request->set_group_id(txn.group_id());
  request->set_flag(txn.flag());
  *request->mutable_belong_shards() = txn.belong_shards();
  request->set_shard_id(txn.shard_id());
  request->set_skip(txn.skip());
  transaction_executor_->AddExecuteMessage(std::move(request));
  return 0;
}

void ThunderboltConsensus::SetupPerformancePreprocessFunc(
    std::function<std::vector<std::string>()> func) {
  performance_manager_->SetPreprocessFunc(func);
}

}  // namespace thunderbolt
}  // namespace resdb
