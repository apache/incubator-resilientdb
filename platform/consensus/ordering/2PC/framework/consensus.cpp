/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "platform/consensus/ordering/2PC/framework/consensus.h"

#include <glog/logging.h>

namespace resdb {
namespace twopc {

Consensus::Consensus(const ResDBConfig& config,
                     std::unique_ptr<TransactionManager> transaction_manager)
    : common::Consensus(config, std::move(transaction_manager)) {
  int id = config.GetSelfInfo().id();
  int f = config.GetMaxMaliciousReplicaNum();
  int total_num = config.GetReplicaInfos().size();

  twopc_ = std::make_unique<TwoPC>(id, f, total_num);

  InitProtocol(twopc_.get());

  coordinator_id_ = config.GetReplicaInfos().empty()
                        ? 1
                        : config.GetReplicaInfos()[0].id();
  twopc_->SetCoordinatorId(coordinator_id_);

  Init();
  eval_start_ = std::chrono::steady_clock::now();
}

int Consensus::ProcessCustomConsensus(std::unique_ptr<Request> request) {
  int msg_type = request->user_type();
  int sender_id = request->sender_id();

  // Broadcast/SendMsg wrap the inner message into request->data().
  // The outer Request only has user_type and sender_id set; hash, seq,
  // and all payload fields belong to the inner message.
  Request inner;
  if (!inner.ParseFromString(request->data())) {
    LOG(ERROR) << "ProcessCustomConsensus: failed to parse inner request";
    return -1;
  }
  std::string txn_id = inner.hash();

  switch (msg_type) {
    case TwoPC::PREPARE:
      return twopc_->ProcessPrepare(txn_id, inner);  // inner, not outer
    case TwoPC::VOTE: {
      bool vote = (inner.seq() == 1);                // inner.seq(), not outer
      return twopc_->ProcessVote(txn_id, sender_id, vote);
    }
    case TwoPC::COMMIT:
      return twopc_->ProcessDecision(txn_id, true);
    case TwoPC::ABORT:
      return twopc_->ProcessDecision(txn_id, false);
    default:
      return 0;
  }
}

int Consensus::ProcessNewTransaction(std::unique_ptr<Request> request) {
  if (config_.GetSelfInfo().id() == coordinator_id_) {
    std::string txn_id = "txn_" + std::to_string(request->proxy_id()) + "_" +
                         std::to_string(request->user_seq());
    return twopc_->StartTransaction(txn_id, *request);
  }
  return SendMsg(Request::TYPE_NEW_TXNS, *request, coordinator_id_);
}

int Consensus::CommitMsg(const google::protobuf::Message& msg) {
  const Request& req = dynamic_cast<const Request&>(msg);

  auto elapsed_s = std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::steady_clock::now() - eval_start_).count();
  uint64_t elapsed = elapsed_s > 0 ? static_cast<uint64_t>(elapsed_s) : 1;

  if (config_.GetSelfInfo().id() == coordinator_id_) {
    uint64_t n = ++coordinator_commits_;
    LOG(INFO) << "[COORDINATOR] commits=" << n
              << " throughput=" << n / elapsed << " tx/s";
  } else {
    uint64_t n = ++participant_commits_;
    LOG(INFO) << "[PARTICIPANT] node=" << config_.GetSelfInfo().id()
              << " commits=" << n
              << " throughput=" << n / elapsed << " tx/s";
  }

  std::unique_ptr<Request> commit_req = std::make_unique<Request>(req);
  transaction_executor_->Commit(std::move(commit_req));
  return 0;
}

}  // namespace twopc
}  // namespace resdb