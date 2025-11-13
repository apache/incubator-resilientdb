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

#include "platform/consensus/ordering/raft/framework/consensus.h"

#include <glog/logging.h>
#include <unistd.h>

#include "common/utils/utils.h"

namespace resdb {
namespace raft {

Consensus::Consensus(const ResDBConfig& config,
                     std::unique_ptr<TransactionManager> executor)
    : common::Consensus(config, std::move(executor)) {
  int total_replicas = config_.GetReplicaNum();
  int f = (total_replicas - 1) / 3;

  Init();

  start_ = 0;

  if (config_.GetPublicKeyCertificateInfo()
          .public_key()
          .public_key_info()
          .type() != CertificateKeyInfo::CLIENT) {
    raft_ = std::make_unique<Raft>(config_.GetSelfInfo().id(), f, total_replicas,
                                 GetSignatureVerifier());
    InitProtocol(raft_.get());
  }
}

int Consensus::ProcessCustomConsensus(std::unique_ptr<Request> request) {
  LOG(ERROR) << "Message type request->user_type(): " << request->user_type();
  if (request->user_type() == MessageType::AppendEntriesMsg) {
    LOG(ERROR) << "Received AppendEntriesMsg";
    std::unique_ptr<AppendEntries> txn = std::make_unique<AppendEntries>();
    if (!txn->ParseFromString(request->data())) {
      LOG(ERROR) << "parse proposal fail";
      assert(1 == 0);
      return -1;
    }
    raft_->ReceivePropose(std::move(txn));
    return 0;
  } else if (request->user_type() == MessageType::AppendEntriesResponseMsg) {
    std::unique_ptr<AppendEntriesResponse> AppendEntriesResponse = std::make_unique<resdb::raft::AppendEntriesResponse>();
    if (!AppendEntriesResponse->ParseFromString(request->data())) {
      LOG(ERROR) << "parse proposal fail";
      assert(1 == 0);
      return -1;
    }
    raft_->ReceiveAppendEntriesResponse(std::move(AppendEntriesResponse));
    return 0;
  }
  return 0;
}

int Consensus::ProcessNewTransaction(std::unique_ptr<Request> request) {
  std::unique_ptr<AppendEntries> txn = std::make_unique<AppendEntries>();
  txn->set_data(request->data());
  txn->set_hash(request->hash());
  txn->set_proxy_id(request->proxy_id());
  txn->set_uid(request->uid());
  return raft_->ReceiveTransaction(std::move(txn));
}

int Consensus::CommitMsg(const google::protobuf::Message& msg) {
  return CommitMsgInternal(dynamic_cast<const AppendEntries&>(msg));
}

int Consensus::CommitMsgInternal(const AppendEntries& txn) {
  std::unique_ptr<Request> request = std::make_unique<Request>();
  request->set_data(txn.data());
  request->set_seq(txn.seq());
  request->set_uid(txn.uid());
  request->set_proxy_id(txn.proxy_id());

  transaction_executor_->Commit(std::move(request));
  return 0;
}

}  // namespace raft
}  // namespace resdb
