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

#include <cstdlib>
#include <glog/logging.h>
#include <set>
#include <thread>
#include <unistd.h>
#include <vector>

#include "common/utils/utils.h"

namespace resdb {
namespace zzy {

namespace {

struct GroupMoveState {
  bool started = false;
};

std::set<int> cur_fail;
std::mutex g_mutex_;
GroupMoveState g_group_move_state;

bool EnableGroupMove() {
  return true;
  const char* flag = std::getenv("RESDB_ENABLE_GROUP_MOVE");
  return flag != nullptr && std::string(flag) == "1";
}

std::vector<int> BuildMoveOrder(int total_replicas) {
  const int f = total_replicas / 3;
  const int group_size = 3;
  std::vector<int> move_order;
  move_order.reserve(total_replicas);
  if (f <= 0) {
    return move_order;
  }

  for (int moved_group = 0; moved_group < f; ++moved_group) {
    const int group_index = (1 + moved_group * 2) % f;
    const int start_id = group_index * group_size + 1;
    const int end_id = std::min(start_id + group_size - 1, total_replicas);
    for (int node_id = start_id; node_id <= end_id; ++node_id) {
      move_order.push_back(node_id);
    }
  }
  return move_order;
}

void StartGroupMoveThread(int total_replicas) {
  const int f = total_replicas / 3;
  const std::vector<int> move_order = BuildMoveOrder(total_replicas);
  std::thread([total_replicas, move_order, f]() {
    int target = 0;
    int f_time = 2;
    const bool use_f1 = true;
    if (use_f1) {
      f_time = 5;
      target = f;
    } else {
      f_time = 5;
      target = total_replicas / 2;
    }

    sleep(5);

    {
      std::unique_lock<std::mutex> lk(g_mutex_);
      for (int node_id = 10; node_id <= total_replicas &&
                              static_cast<int>(cur_fail.size()) < target;
           ++node_id) {
        LOG(ERROR) << "move node:" << node_id;
        cur_fail.insert(node_id);
      }
    }

    sleep(f_time);

    {
      std::unique_lock<std::mutex> lk(g_mutex_);
      while (!cur_fail.empty()) {
        LOG(ERROR) << "recover node:" << *cur_fail.begin();
        cur_fail.erase(cur_fail.begin());
      }
    }
  }).detach();
}

bool IsCrossPartitionBlocked(int self_id, int sender_id) {
  const bool self_isolated = cur_fail.find(self_id) != cur_fail.end();
  const bool sender_isolated = cur_fail.find(sender_id) != cur_fail.end();
  return self_isolated != sender_isolated;
}

}  // namespace

std::unique_ptr<ZZYPerformanceManager> Consensus::GetPerformanceManager() {
  return config_.IsPerformanceRunning()
             ? std::make_unique<ZZYPerformanceManager>(
                   config_, GetBroadCastClient(), GetSignatureVerifier())
             : nullptr;
}

Consensus::Consensus(const ResDBConfig& config,
                     std::unique_ptr<TransactionManager> executor)
    : common::Consensus(config, std::move(executor)) {
  const int total_replicas = config_.GetReplicaNum();
  const int f = (total_replicas - 1) / 3;

  if (config_.GetPublicKeyCertificateInfo()
          .public_key()
          .public_key_info()
          .type() == CertificateKeyInfo::CLIENT) {
    SetPerformanceManager(GetPerformanceManager());
  }

  Init();
  global_stats_ = Stats::GetGlobalStats();

  if (config_.GetPublicKeyCertificateInfo()
          .public_key()
          .public_key_info()
          .type() != CertificateKeyInfo::CLIENT) {
    zzy_ = std::make_unique<ZZY>(config_, config_.GetSelfInfo().id(), f,
                                 total_replicas, 
                                 config_.GetConfigData().block_size(),
                                 GetSignatureVerifier());
    InitProtocol(zzy_.get());
    zzy_->SetFailFunc([&](const Transaction& txn) {
      SendFail(txn.proxy_id(), txn.hash());
    });
    zzy_->SetSpeculativeExecuteFunc(
        [&](const Transaction& txn) { return SpeculativeExecute(txn); });
    zzy_->SetClientCompleteFunc([&](int64_t local_id) {
      LOG(ERROR) << "zzy client completed local_id:" << local_id;
    });
  }
}

int Consensus::ProcessCustomConsensus(std::unique_ptr<Request> request) {
  if (EnableGroupMove()) {
    std::unique_lock<std::mutex> lk(g_mutex_);
    if (!g_group_move_state.started) {
      g_group_move_state.started = true;
      cur_fail.clear();
      StartGroupMoveThread(config_.GetReplicaNum());
    }

    if (IsCrossPartitionBlocked(config_.GetSelfInfo().id(),
                                request->sender_id())) {
      LOG(ERROR) << "skip message by isolation, sender:" << request->sender_id()
                 << " self:" << config_.GetSelfInfo().id();
      global_stats_->SeqGap(cur_fail.size());
      return 0;
    }
    global_stats_->SeqGap(cur_fail.size());
  }

  if (request->user_type() == MessageType::ProposeBatch) {
    auto batch = std::make_unique<TransactionBatch>();
    if (!batch->ParseFromString(request->data())) {
      LOG(ERROR) << "parse proposal batch fail";
      return -1;
    }
    return zzy_->ReceiveProposeBatch(std::move(batch)) ? 0 : -1;
  }

  if (request->user_type() == MessageType::Propose) {
    auto txn = std::make_unique<Transaction>();
    if (!txn->ParseFromString(request->data())) {
      LOG(ERROR) << "parse proposal fail";
      return -1;
    }
    return zzy_->ReceivePropose(std::move(txn)) ? 0 : -1;
  }

  if (request->user_type() == MessageType::Prepare) {
    auto proposal = std::make_unique<Proposal>();
    if (!proposal->ParseFromString(request->data())) {
      LOG(ERROR) << "parse prepare fail";
      return -1;
    }
    return zzy_->ReceivePrepare(std::move(proposal)) ? 0 : -1;
  }

  if (request->user_type() == MessageType::CommitACK) {
    auto proposal = std::make_unique<Proposal>();
    if (!proposal->ParseFromString(request->data())) {
      LOG(ERROR) << "parse commit ack fail";
      return -1;
    }
    return zzy_->ReceiveClientCommitACK(std::move(proposal)) ? 0 : -1;
  }

  if (request->user_type() == MessageType::ACK) {
    auto proposal = std::make_unique<Proposal>();
    if (!proposal->ParseFromString(request->data())) {
      LOG(ERROR) << "parse order reply fail";
      return -1;
    }
    return zzy_->ReceiveOrderReply(std::move(proposal)) ? 0 : -1;
  }

  if (request->user_type() == MessageType::Commit) {
    auto proposal = std::make_unique<Proposal>();
    if (!proposal->ParseFromString(request->data())) {
      LOG(ERROR) << "parse commit fail";
      return -1;
    }
    return zzy_->ReceiveCommit(std::move(proposal)) ? 0 : -1;
  }

  return 0;
}

int Consensus::ProcessNewTransaction(std::unique_ptr<Request> request) {
  auto txn = std::make_unique<Transaction>();
  txn->set_data(request->data());
  txn->set_hash(request->hash());
  txn->set_proxy_id(request->proxy_id());
  txn->set_uid(request->uid());

  const int proxy_id = txn->proxy_id();
  const std::string hash = txn->hash();
  const bool ret = zzy_->ReceiveTransaction(std::move(txn));
  if (!ret) {
    SendFail(proxy_id, hash);
  }
  return ret ? 0 : -1;
}

int Consensus::CommitMsg(const google::protobuf::Message& msg) {
  return CommitMsgInternal(dynamic_cast<const Transaction&>(msg));
}

int Consensus::CommitMsgInternal(const Transaction& txn) {
  auto request = std::make_unique<Request>();
  request->set_data(txn.data());
  request->set_seq(txn.seq());
  request->set_uid(txn.uid());
  request->set_proxy_id(txn.proxy_id());
  request->set_hash(txn.hash());

  transaction_executor_->Commit(std::move(request));
  return 0;
}

int Consensus::SpeculativeExecute(const Transaction& txn) {
  auto request = std::make_unique<Request>();
  request->set_data(txn.data());
  request->set_seq(txn.seq());
  request->set_uid(txn.uid());
  request->set_proxy_id(txn.proxy_id());
  request->set_hash(txn.hash());
  request->set_create_time(txn.create_time());

  transaction_executor_->Prepare(std::move(request));
  return 0;
}

}  // namespace zzy
}  // namespace resdb
