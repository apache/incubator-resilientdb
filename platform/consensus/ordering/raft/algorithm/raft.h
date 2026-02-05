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

#pragma once

#include <sys/types.h>
#include <cstdint>
#include <deque>
#include <map>
#include <memory>
#include <queue>
#include <thread>
#include <chrono>

#include "platform/common/queue/lock_free_queue.h"
#include "platform/consensus/ordering/common/algorithm/protocol_base.h"
#include "platform/consensus/ordering/raft/proto/proposal.pb.h"
#include "platform/proto/resdb.pb.h"
#include "platform/statistic/stats.h"
#include "platform/consensus/ordering/raft/algorithm/leaderelection_manager.h"
#include "platform/networkstrate/replica_communicator.h"

namespace resdb {
namespace raft {

enum class Role { FOLLOWER, CANDIDATE, LEADER };
enum class TermRelation { STALE, CURRENT, NEW };

class LogEntry {
  public:
  uint64_t term;
  std::string command;

  uint32_t GetSerializedSize();
  uint32_t ComputeSerializedEntrySize() const;
  
  private:
  uint32_t serializedSize = 0;
};

struct AeFields {
  uint64_t term = 0;
  int leaderId = -1;
  uint64_t prevLogIndex = 0;
  uint64_t prevLogTerm = 0;
  std::vector<LogEntry> entries{};
  uint64_t leaderCommit = 0;
  int followerId = -1; // not part of AE message itself, but needed to determine recipient
};

struct InFlightMsg {
  std::chrono::steady_clock::time_point timeSent;
  uint64_t prevLogIndexSent;
  uint64_t lastIndexOfSegmentSent;
};

class Raft : public common::ProtocolBase {
 public:
  Raft(int id, int f, int total_num,
    SignatureVerifier* verifier,
    LeaderElectionManager* leaderelection_manager,
    ReplicaCommunicator* replica_communicator
  );
  ~Raft();

  const bool replicationLoggingFlag_ = true;
  const bool livenessLoggingFlag_ = false;

  bool ReceiveTransaction(std::unique_ptr<Request> req);
  bool ReceiveAppendEntries(std::unique_ptr<AppendEntries> ae);
  bool ReceiveAppendEntriesResponse(std::unique_ptr<AppendEntriesResponse> aer);
  void ReceiveRequestVote(std::unique_ptr<RequestVote> rv);
  void ReceiveRequestVoteResponse(std::unique_ptr<RequestVoteResponse> rvr);
  void StartElection();
  void SendHeartBeat();
  Role GetRoleSnapshot() const;
  void SetRole(Role role);

 private:
  mutable std::mutex mutex_;

  TermRelation TermCheckLocked(uint64_t term) const;  // Must be called under mutex
  bool DemoteSelfLocked(uint64_t term); // Must be called under mutex
  uint64_t getLastLogTermLocked() const; // Must be called under mutex
  bool IsStop();
  //bool IsDuplicateLogEntry(const std::string& hash) const; // Must be called under mutex
  std::vector<std::unique_ptr<Request>> PrepareCommitLocked(); // Must be called under mutex
  AeFields GatherAeFieldsLocked(int followerId, bool heartBeat = false) const; // Must be called under mutex
  std::vector<AeFields> GatherAeFieldsForBroadcastLocked(bool heartBeat = false) const; // Must be called under mutex
  void CreateAndSendAppendEntryMsg(const AeFields& fields);
  LogEntry CreateLogEntry(const Entry& entry) const;
  void ClearInFlightsLocked();
  void PruneExpiredInFlightMsgsLocked();
  void PruneRedundantInFlightMsgsLocked(int followerId, uint64_t followerLastLogIndex);
  void RecordNewInFlightMsgLocked(const AeFields& msg, std::chrono::steady_clock::time_point timestamp);
  bool InFlightPerFollowerLimitReachedLocked(int followerId) const;

  
  // Persistent state on all servers:
  uint64_t currentTerm_; // Protected by mutex_
  int votedFor_; // Protected by mutex_
  std::vector<std::unique_ptr<LogEntry>> log_; // Protected by mutex_

  // Volatile state on leaders:
  std::vector<uint64_t> nextIndex_; // Protected by mutex_
  std::vector<uint64_t> matchIndex_; // Protected by mutex_
  uint64_t heartBeatsSentThisTerm_; // Protected by mutex_
  uint64_t lastLogIndex_; // Protected by mutex_

  // Volatile state on all servers:
  uint64_t commitIndex_; // Protected by mutex_
  uint64_t lastApplied_; // Protected by mutex_
  Role role_; // Protected by mutex_
  //int leaderId_; // Protected by mutex_
  std::vector<int> votes_; // Protected by mutex_
  std::vector<std::vector<InFlightMsg>> inflightVecs_; // Protected by mutex_
  //std::chrono::steady_clock::time_point last_ae_time_;
  //std::chrono::steady_clock::time_point last_heartbeat_time_; // Protected by mutex_

  bool is_stop_;
  const uint64_t quorum_;

  // for limiting AppendEntries batch sizing
  static constexpr size_t maxHeaderBytes = 64;
  static constexpr size_t maxBytes = 64 * 1024;
  static constexpr size_t maxEntries = 16;
  static constexpr size_t maxInFlightPerFollower = 4;
  static constexpr std::chrono::milliseconds AEResponseDeadline{300}; // in milliseconds
  
  SignatureVerifier* verifier_;
  LeaderElectionManager* leader_election_manager_;
  //Stats* global_stats_;
  ReplicaCommunicator* replica_communicator_;
};

}  // namespace raft
}  // namespace resdb
