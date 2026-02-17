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
#ifdef RAFT_TEST_MODE
#include <ostream>
#endif

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

#ifdef RAFT_TEST_MODE
struct RaftStatePatch {
  std::optional<uint64_t> currentTerm;
  std::optional<int> votedFor;
  std::optional<uint64_t> commitIndex;
  std::optional<uint64_t> lastApplied;
  std::optional<Role> role;

  std::optional<std::vector<std::unique_ptr<LogEntry>>> log;
  std::optional<std::vector<uint64_t>> nextIndex;
  std::optional<std::vector<uint64_t>> matchIndex;
  std::optional<std::vector<int>> votes;
};
#endif

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

  virtual bool ReceiveTransaction(std::unique_ptr<Request> req);
  virtual bool ReceiveAppendEntries(std::unique_ptr<AppendEntries> ae);
  virtual bool ReceiveAppendEntriesResponse(std::unique_ptr<AppendEntriesResponse> aer);
  virtual void ReceiveRequestVote(std::unique_ptr<RequestVote> rv);
  virtual void ReceiveRequestVoteResponse(std::unique_ptr<RequestVoteResponse> rvr);
  virtual void StartElection();
  virtual void SendHeartBeat();
  virtual Role GetRoleSnapshot() const;
  virtual void SetRole(Role role);
  virtual void PrintDebugState() const;

 private:
  mutable std::mutex mutex_;

  virtual TermRelation TermCheckLocked(uint64_t term) const;  // Must be called under mutex
  virtual bool DemoteSelfLocked(uint64_t term); // Must be called under mutex
  virtual uint64_t getLastLogTermLocked() const; // Must be called under mutex
  virtual bool IsStop();
  //bool IsDuplicateLogEntry(const std::string& hash) const; // Must be called under mutex
  virtual std::vector<std::unique_ptr<Request>> PrepareCommitLocked(); // Must be called under mutex
  virtual AeFields GatherAeFieldsLocked(int followerId, bool heartBeat = false) const; // Must be called under mutex
  std::vector<AeFields> GatherAeFieldsForBroadcastLocked(bool heartBeat = false) const; // Must be called under mutex
  virtual void CreateAndSendAppendEntryMsg(const AeFields& fields);
  virtual LogEntry CreateLogEntry(const Entry& entry) const;
  virtual void ClearInFlightsLocked();
  virtual void PruneExpiredInFlightMsgsLocked();
  virtual void PruneRedundantInFlightMsgsLocked(int followerId, uint64_t followerLastLogIndex);
  virtual void RecordNewInFlightMsgLocked(const AeFields& msg, std::chrono::steady_clock::time_point timestamp);
  virtual bool InFlightPerFollowerLimitReachedLocked(int followerId) const;

  
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

#ifdef RAFT_TEST_MODE
 public:
  void SetStateForTest(RaftStatePatch patch) {
    std::lock_guard lk(mutex_);

    if (patch.currentTerm)  currentTerm_  = *patch.currentTerm;
    if (patch.votedFor)     votedFor_     = *patch.votedFor;
    if (patch.commitIndex)  commitIndex_  = *patch.commitIndex;
    if (patch.lastApplied)  lastApplied_  = *patch.lastApplied;
    if (patch.role)         role_         = *patch.role;

    if (patch.log) {
      log_ = std::move(*patch.log);
      lastLogIndex_ = log_.empty() ? 0 : log_.size() - 1;
    }

    if (patch.nextIndex)   nextIndex_  = *patch.nextIndex;
    if (patch.matchIndex)  matchIndex_ = *patch.matchIndex;
    if (patch.votes)       votes_      = *patch.votes;
  }

  uint64_t GetCurrentTerm() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return currentTerm_;
  }

  int GetVotedFor() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return votedFor_;
  }

  const std::vector<std::unique_ptr<LogEntry>>& GetLog() const  {
    std::lock_guard<std::mutex> lock(mutex_);
    return log_;
  }
 
  void PrintLog(std::ostream& os) const {
    os << "Log entries (count = " << log_.size() << "):\n";

    for (size_t i = 0; i < log_.size(); ++i) {
      const auto& entry = log_[i];
      if (!entry) {
        os << "  [" << i << "] <null entry>\n";
        continue;
      }

      os << "  [" << i << "] "
        << "term=" << entry->term
        << ", command=\"" << entry->command << "\""
        << ", serializedSize=" << entry->GetSerializedSize()
        << "\n";
    }
  }

  size_t GetLogSize() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return log_.size();
  }

  uint64_t GetLastLogIndexFromLog() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return log_.empty() ? 0 : log_.size() - 1;
  }

  std::vector<uint64_t> GetNextIndex() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return nextIndex_;
  }

  std::vector<uint64_t> GetMatchIndex() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return matchIndex_;
  }

  uint64_t GetHeartBeatsSentThisTerm() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return heartBeatsSentThisTerm_;
  }

  uint64_t GetLastLogIndex() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return lastLogIndex_;
  }

  uint64_t GetCommitIndex() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return commitIndex_;
  }

  uint64_t GetLastApplied() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return lastApplied_;
  }

  Role GetRole() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return role_;
  }

  std::vector<int> GetVotes() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return votes_;
  }

  std::vector<std::vector<InFlightMsg>> GetInFlightVecs() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return inflightVecs_;
  }

#endif
};

}  // namespace raft
}  // namespace resdb
