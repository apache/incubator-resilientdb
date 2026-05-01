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
#include "platform/consensus/ordering/raft/proto/proposal.pb.h"
#include "platform/proto/resdb.pb.h"

namespace resdb {
namespace raft {

Consensus::Consensus(const ResDBConfig& config,
                     std::unique_ptr<TransactionManager> executor)
    : common::Consensus(config, std::move(executor)),
      leader_election_manager_(
          std::make_unique<LeaderElectionManager>(config_)),
      system_info_(std::make_unique<SystemInfo>(config_)),
      raft_checkpoint_manager_(std::make_unique<RaftCheckPoint>()),
      recovery_(std::make_unique<RaftRecovery>(
          config_, raft_checkpoint_manager_.get(),
          transaction_executor_->GetStorage(),
          [this](uint64_t seq) { OnCheckpointFinish(seq); })) {
  //LOG(INFO) << "JIM -> " << __FUNCTION__ << ": In consensus constructor";
  int total_replicas = config_.GetReplicaNum();
  int f = (total_replicas - 1) / 3;

  Init();

  if (config_.GetPublicKeyCertificateInfo()
          .public_key()
          .public_key_info()
          .type() != CertificateKeyInfo::CLIENT) {
    raft_ = std::make_unique<Raft>(config_.GetSelfInfo().id(), f, total_replicas,
                                 GetSignatureVerifier(), leader_election_manager_.get(),
                                 replica_communicator_, recovery_.get());

    leader_election_manager_->SetRaft(raft_.get());
    leader_election_manager_->MayStart();

    RecoverFromLogs();

    InitProtocol(raft_.get());
  }
}

int Consensus::ProcessCustomConsensus(std::unique_ptr<Request> request) {
  if (request->user_type() == MessageType::AppendEntriesMsg) {
    //LOG(ERROR) << "Received AppendEntriesMsg";
    std::unique_ptr<AppendEntries> txn = std::make_unique<AppendEntries>();
    if (!txn->ParseFromString(request->data())) {
      LOG(ERROR) << "parse proposal fail";
      assert(1 == 0);
      return -1;
    }
    raft_->ReceiveAppendEntries(std::move(txn));
    return 0;
  }
  else if (request->user_type() == MessageType::AppendEntriesResponseMsg) {
    std::unique_ptr<AppendEntriesResponse> AppendEntriesResponse = std::make_unique<resdb::raft::AppendEntriesResponse>();
    if (!AppendEntriesResponse->ParseFromString(request->data())) {
      LOG(ERROR) << "parse proposal fail";
      assert(1 == 0);
      return -1;
    }
    raft_->ReceiveAppendEntriesResponse(std::move(AppendEntriesResponse));
    return 0;
  }
  else if (request->user_type() == MessageType::RequestVoteMsg) {
    std::unique_ptr<RequestVote> rv = std::make_unique<resdb::raft::RequestVote>();
    if (!rv->ParseFromString(request->data())) {
      LOG(ERROR) << "parse proposal fail";
      assert(1 == 0);
      return -1;
    }
    raft_->ReceiveRequestVote(std::move(rv));
    return 0;
  }
  else if (request->user_type() == MessageType::RequestVoteResponseMsg) {
    std::unique_ptr<RequestVoteResponse> rvr = std::make_unique<resdb::raft::RequestVoteResponse>();
    if (!rvr->ParseFromString(request->data())) {
      LOG(ERROR) << "parse proposal fail";
      assert(1 == 0);
      return -1;
    }
    raft_->ReceiveRequestVoteResponse(std::move(rvr));
    return 0;
  }
  else if (request->user_type() == MessageType::DirectToLeaderMsg) {
    //LOG(INFO) << "JIM -> " << __FUNCTION__ << ": In DirectToLeader";
    std::unique_ptr<DirectToLeader> dtl = std::make_unique<resdb::raft::DirectToLeader>();
    if (!dtl->ParseFromString(request->data())) {
      LOG(ERROR) << "parse proposal fail";
      assert(1 == 0);
      return -1;
    }
    performance_manager_->SetPrimary(dtl->leaderid());
    return 0;
  }
  LOG(ERROR) << "Unknown message type";
  return 0;
}

void Consensus::RecoverFromLogs() {
  recovery_->ReadLogs(
      [&](const RaftMetadata& metadata) {
        LOG(INFO) << " read current term: " << metadata.current_term
                  << " voted for: " << metadata.voted_for;
        raft_->SetCurrentTerm(metadata.current_term, false);
        raft_->SetVotedFor(metadata.voted_for, false);
        raft_->SetSnapshotLastIndexAndTerm(metadata.snapshot_last_index,
                                           metadata.snapshot_last_term, false);
      },
      [&](std::unique_ptr<WALRecord> record) {
        switch (record->payload_case()) {
          case WALRecord::kEntry: {
            LogEntry logEntry;
            logEntry.entry = record->entry();
            raft_->AddToLog(logEntry, false);
            break;
          }
          case WALRecord::kTruncation:
            raft_->TruncateLog(record->truncation().truncate_from_index(),
                               false);
            break;
          case WALRecord::PAYLOAD_NOT_SET:
            assert(false && "WALRecord does not contain Truncation or Entry");
            break;
        }
      },
      [](int) {});
}

int Consensus::ProcessNewTransaction(std::unique_ptr<Request> request) {
    return raft_->ReceiveTransaction(std::move(request));
}

int Consensus::CommitMsg(const google::protobuf::Message& msg) {
  auto* req = dynamic_cast<const Request*>(&msg);
  if (!req) {
    LOG(INFO) << "JIM -> " << __FUNCTION__ << ": Failed to cast Message to Request";
    return -1;
  }
  auto execReq = std::make_unique<Request>(*req);
  transaction_executor_->Commit(std::move(execReq));
  return 0;
}

int Consensus::ResponseMsg(const BatchUserResponse& batch_resp) {
  // While we may receive these ResponseMsg's out of order, we do know the
  // execution of transactions are guaranteed to be in order, so we know all
  // transactions before batch_resp.seq() have been executed.
  last_applied_ = std::max(batch_resp.seq(), last_applied_);

  // raft_checkpoint_manager_->SetStableCheckpoint(batch_resp.seq());
  if (batch_resp.seq() >= snapshot_interval_ + last_snapshot_initiated_at_) {
    LOG(INFO) << "Initiating checkpoint at seq: " << batch_resp.seq();
    // Update the checkpoint in the manager
    raft_checkpoint_manager_->SetStableCheckpoint(batch_resp.seq());
    last_snapshot_initiated_at_ = batch_resp.seq();
    LOG(INFO) << "Next Checkpoint will be after "
              << (snapshot_interval_ + last_snapshot_initiated_at_);
  }
  return common::Consensus::ResponseMsg(batch_resp);
};

void Consensus::OnCheckpointFinish(uint64_t seq) {
  LOG(INFO) << "Checkpointed all entries up to " << seq;
  // raft_->TruncatePrefix(seq);
}

}  // namespace raft
}  // namespace resdb
