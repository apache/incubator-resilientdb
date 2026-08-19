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

#include <chrono>
#include <cstdint>
#include <deque>
#include <future>
#include <map>
#include <memory>
#include <queue>
#include <thread>
#ifdef RAFT_TEST_MODE
#include <ostream>
#endif

#include "platform/common/queue/lock_free_queue.h"
#include "platform/consensus/ordering/common/algorithm/protocol_base.h"
#include "platform/consensus/ordering/raft/algorithm/leader_election_manager.h"
#include "platform/consensus/ordering/raft/proto/proposal.pb.h"
#include "platform/consensus/recovery/raft_recovery.h"
#include "platform/networkstrate/replica_communicator.h"
#include "platform/proto/resdb.pb.h"
#include "platform/statistic/stats.h"

#ifdef RAFT_TEST_MODE
#include <google/protobuf/util/message_differencer.h>
#endif
namespace resdb {
namespace raft {

enum class Role { FOLLOWER, CANDIDATE, LEADER };
enum class TermRelation { STALE, CURRENT, NEW };
enum class ProgressState { PROBE, REPLICATE, SNAPSHOT };

#define RAFT_DEBUG

#ifdef RAFT_DEBUG
#include <sys/resource.h>
#include <sys/sysinfo.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

class ResourceMonitor {
 public:
  ResourceMonitor() : last_(TakeSample()) {}

  std::string GetPageFaults() {
    struct rusage usage;
    std::ostringstream ss;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
      // ru_minflt: Minor faults (no disk I/O, e.g., lazy memory allocation)
      // ru_majflt: Major faults (required loading data from storage drive)
      ss << "Minor Page Faults: " << usage.ru_minflt << "\n";
      ss << "Major Page Faults: " << usage.ru_majflt << "\n";
    }

    return ss.str();
  }

  std::string GetStatus() {
    Sample current = TakeSample();

    double elapsed =
        std::chrono::duration<double>(current.wall - last_.wall).count();

    double cpu = 0.0;
    if (elapsed > 0) {
      cpu = 100.0 *
            ((current.cpu_ticks - last_.cpu_ticks) /
             static_cast<double>(ticks_per_second_)) /
            elapsed;
    }

    last_ = current;

    double loads[3] = {0};
    getloadavg(loads, 3);

    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1);
    ss << "CPU=" << cpu << "% "
       << "RSS=" << current.rss_mb << "MB "
       << "VM=" << current.vm_mb << "MB "
       << "Threads=" << current.threads << " "
       << "LoadAvg=" << loads[0] << " " << GetPageFaults();

    return ss.str();
  }

 private:
  struct Sample {
    long cpu_ticks = 0;
    size_t rss_mb = 0;
    size_t vm_mb = 0;
    int threads = 0;
    std::chrono::steady_clock::time_point wall;
  };

  Sample last_;
  const long ticks_per_second_ = sysconf(_SC_CLK_TCK);

  Sample TakeSample() {
    Sample s;
    s.wall = std::chrono::steady_clock::now();

    // Read utime/stime from /proc/self/stat
    {
      std::ifstream stat("/proc/self/stat");
      std::string tmp;

      for (int i = 0; i < 13; ++i) stat >> tmp;

      long utime, stime;
      stat >> utime >> stime;
      s.cpu_ticks = utime + stime;
    }

    // Read memory/thread info
    {
      std::ifstream status("/proc/self/status");
      std::string key;

      while (status >> key) {
        if (key == "VmRSS:") {
          size_t kb;
          status >> kb;
          s.rss_mb = kb / 1024;
        } else if (key == "VmSize:") {
          size_t kb;
          status >> kb;
          s.vm_mb = kb / 1024;
        } else if (key == "Threads:") {
          status >> s.threads;
        }

        std::string line;
        std::getline(status, line);
      }
    }

    return s;
  }
};

#endif

class LogEntry {
 public:
  Entry entry;

  uint32_t GetSerializedSize() const;
  uint32_t ComputeSerializedEntrySize() const;

#ifdef RAFT_TEST_MODE
  bool operator==(const LogEntry& other) const {
    return google::protobuf::util::MessageDifferencer::Equals(entry,
                                                              other.entry);
  }

  bool operator!=(const LogEntry& other) const { return !(*this == other); }
#endif

 private:
  mutable uint32_t serialized_size = 0;
};

struct AeFields {
  uint64_t term = 0;
  int leader_id = -1;
  uint64_t prev_log_index = 0;
  uint64_t prev_log_term = 0;
  std::vector<LogEntry> entries{};
  uint64_t leader_commit = 0;
  // Not part of AE message itself, but needed to determine recipient
  int follower_id = -1;
};

struct InFlightMsg {
  std::chrono::steady_clock::time_point time_sent;
  uint64_t prev_log_index_sent;
  uint64_t last_index_of_segment_sent;
};

struct FollowerProgress {
  ProgressState state = ProgressState::PROBE;
  uint64_t match_index = 0;
  uint64_t next_index = 1;
  std::vector<InFlightMsg> in_flight;
  bool probe_in_flight = false;
};

struct ReceiveTransactionResult {
  bool direct_to_leader = false;
  bool broadcasted = false;
  uint64_t my_log_index = 0;
  std::future<void> wal_future;
  std::vector<AeFields> messages;
};

struct ReceiveAppendEntriesResult {
  // success is true if the AppendEntries is not from a stale term, and the
  // follower has an entry at prev_log_index that matches prev_log_term.
  bool success = false;
  bool demoted = false;
  uint64_t term = 0;
  TermRelation tr;
  uint64_t last_log_index = 0;
  Role initial_role = Role::FOLLOWER;
  std::future<void> wal_future;
  std::vector<std::unique_ptr<Request>> entries_to_apply;
  uint64_t conflicting_term = 0;
  uint64_t conflicting_index = 0;
};

struct ReceiveAppendEntriesResponseResult {
  bool demoted = false;
  uint64_t term = 0;
  Role initial_role = Role::FOLLOWER;
  bool resending = false;
  bool should_send_snapshot = false;
  std::vector<std::unique_ptr<Request>> entries_to_apply;
  std::vector<AeFields> fields_vector;
  int follower_id = -1;
};

struct ReceiveRequestVoteResult {
  bool demoted = false;
  uint64_t term = 0;
  Role initial_role = Role::FOLLOWER;
  bool vote_granted = false;
  bool valid_candidate = false;
  int voted_for = -1;
};

struct ReceiveRequestVoteResponseResult {
  bool demoted = false;
  bool elected = false;
  uint64_t term = 0;
  Role initial_role = Role::FOLLOWER;
};

#ifdef RAFT_TEST_MODE

struct FollowerProgressPatch {
  std::optional<ProgressState> state;
  std::optional<uint64_t> match_index;
  std::optional<uint64_t> next_index;
  std::optional<std::vector<InFlightMsg>> in_flight;
};

struct RaftStatePatch {
  std::optional<uint64_t> current_term;
  std::optional<int> voted_for;
  std::optional<uint64_t> commit_index;
  std::optional<uint64_t> last_committed;
  std::optional<Role> role;

  std::optional<uint64_t> snapshot_last_index;
  std::optional<uint64_t> snapshot_last_term;
  std::optional<uint64_t> truncated_last_index;
  std::optional<uint64_t> truncated_last_term;

  std::optional<std::vector<LogEntry>> log;
  std::optional<std::vector<FollowerProgressPatch>> progress;
  std::optional<std::vector<int>> votes;
  std::optional<bool> enable_batching;
  std::optional<uint64_t> snapshot_buffer_amount;
  std::optional<size_t> max_in_flight_per_follower;
};

#endif

class Raft : public common::ProtocolBase {
 public:
  Raft(int id, int f, int total_num, SignatureVerifier* verifier,
       LeaderElectionManager* leader_election_manager,
       ReplicaCommunicator* replica_communicator, RaftRecovery* recovery,
       const ResDBConfig& config);
  ~Raft();

  virtual bool ReceiveTransaction(std::unique_ptr<Request> req);
  virtual bool ReceiveAppendEntries(std::unique_ptr<AppendEntries> ae);
  virtual bool ReceiveAppendEntriesResponse(
      std::unique_ptr<AppendEntriesResponse> aer);
  virtual void ReceiveRequestVote(std::unique_ptr<RequestVote> rv);
  virtual void ReceiveRequestVoteResponse(
      std::unique_ptr<RequestVoteResponse> rvr);
  virtual bool ReceiveInstallSnapshot(std::unique_ptr<InstallSnapshot> is);
  virtual bool ReceiveInstallSnapshotResponse(
      std::unique_ptr<InstallSnapshotResponse> isr);
  virtual void StartElection();
  virtual void SendHeartbeat();
  virtual Role GetRoleSnapshot() const;
  virtual void SetRole(Role role);
  virtual void PrintDebugState() const;
  void WriteMetadataLocked();
  uint64_t GetSnapshotLastIndex();

  // These functions with write_metadata are also used to replay information
  // upon recovery. So, they are called with false during recovery, and true
  // everywhere else.
  virtual void SetCurrentTerm(uint64_t current_term,
                              bool write_metadata = true);
  virtual void SetCurrentTermLocked(uint64_t current_term,
                                    bool write_metadata = true);
  virtual void SetVotedFor(int votedFor, bool write_metadata = true);
  virtual void SetVotedForLocked(int votedFor, bool write_metadata = true);
  virtual void SetCurrentTermAndVotedFor(uint64_t current_term, int voted_for,
                                         bool write_metadata = true);
  virtual void SetCurrentTermAndVotedForLocked(uint64_t current_term,
                                               int voted_for,
                                               bool write_metadata = true);
  void SetSnapshotLastIndexAndTermLocked(uint64_t snapshot_last_index,
                                         uint64_t snapshot_last_term,
                                         uint64_t truncated_last_index,
                                         uint64_t truncated_last_term,
                                         bool write_metadata = true);
  void SetSnapshotLastIndexAndTerm(uint64_t snapshot_last_index,
                                   uint64_t snapshot_last_term,
                                   bool write_metadata = true);
  void AddToLog(LogEntry& log_entry, bool write_metadata = true);
  std::future<void> AddToLogLocked(LogEntry& log_entry,
                                   bool write_metadata = true);
  void AddToLog(std::vector<LogEntry> logEntriesToAdd,
                bool write_metadata = true);
  std::future<void> AddToLogLocked(std::vector<LogEntry> logEntriesToAdd,
                                   bool write_metadata = true);
  void TruncateLog(uint64_t first, bool write_metadata = true);
  void TruncateLogLocked(uint64_t first, bool write_metadata = true);
  void TruncatePrefix(uint64_t snapshot_index);
  bool IsSendSnapshotInProgress() {
    std::lock_guard<std::mutex> lk(mutex_);
    for (const auto& [last_sent_time, _] : snapshot_send_time_) {
      if (last_sent_time != std::chrono::steady_clock::time_point{}) {
        return true;
      }
    }
    return false;
  }
  // These flags gate logging inside hot paths. replication_logging_flag_ is
  // on by default; liveness_logging_flag_ adds heartbeat/timing noise and is
  // off unless you're actively debugging liveness.
  const bool replication_logging_flag_ = true;
  const bool liveness_logging_flag_ = false;

 private:
  mutable std::mutex mutex_;
  mutable std::mutex snapshot_queue_mutex_;
  std::condition_variable snapshot_queue_cv_;

  ReceiveTransactionResult ReceiveTransactionLocked(
      const std::unique_ptr<Request>& req, const std::string& serialized);
  ReceiveAppendEntriesResult ReceiveAppendEntriesLocked(
      const std::unique_ptr<AppendEntries>& ae);
  ReceiveAppendEntriesResponseResult ReceiveAppendEntriesResponseLocked(
      const std::unique_ptr<AppendEntriesResponse>& aer);
  ReceiveRequestVoteResult ReceiveRequestVoteLocked(
      const std::unique_ptr<RequestVote>& rv);
  ReceiveRequestVoteResponseResult ReceiveRequestVoteResponseLocked(
      const std::unique_ptr<RequestVoteResponse>& rvr);

  virtual TermRelation TermCheckLocked(
      uint64_t term) const;                       // Must be called under mutex
  virtual bool DemoteSelfLocked(uint64_t term);   // Must be called under mutex
  virtual uint64_t GetLastLogTermLocked() const;  // Must be called under mutex
  virtual bool IsStop();
  virtual std::vector<std::unique_ptr<Request>>
  PrepareCommitLocked();  // Must be called under mutex
  virtual AeFields GatherAeFieldsLocked(
      int follower_id);  // Must be called under mutex
  std::vector<AeFields> GatherAeFieldsForBroadcastLocked(
      bool heartbeat = false);  // Must be called under mutex
  virtual void CreateAndSendAppendEntryMsg(const AeFields& fields);
  virtual LogEntry CreateLogEntry(const Entry& entry) const;
  virtual void ClearInFlightsLocked();
  // This function is called before sending any AppendEntries messages to
  // followers. Once messages have been in a follower's
  // FollowerProgress.in_flight for longer than ae_response_deadline_, remove
  // them so that they will be re-sent.
  virtual void PruneExpiredInFlightMsgsLocked();
  virtual void PruneRedundantInFlightMsgsLocked(
      int follower_id,
      uint64_t follower_last_log_index);  // Must be called under mutex_.
  virtual void RecordNewInFlightMsgLocked(
      const AeFields& msg, std::chrono::steady_clock::time_point
                               timestamp);     // Must be called under mutex_.
  virtual void PrintDebugStateLocked() const;  // Must be called under mutex_.
  void CheckSnapshotQueue();
  void EnqueueSnapshot(int follower_id, size_t byte_offset);
  void EnqueueSnapshotLocked(int follower_id, size_t byte_offset);
  // Used to drain the snapshot queue when a leader demotes.
  void RequestSnapshotQueueDrain();
  bool ShouldSendSnapshotChunkLocked(int follower_id, size_t byte_offset)
      const;  // Must be called under mutex_.

#ifdef RAFT_TEST_MODE
 public:
  std::string GetSnapshotFilePath() const { return snapshot_file_path_; }
#endif
  bool CanSendLocked(int follower_id) const;
  uint64_t GetLogicalLogSize() const;
  const LogEntry& GetLogEntryAtIndex(uint64_t index) const;
  const uint64_t GetLogTermAtIndex(uint64_t index) const;
  void SendInstallSnapshot(int follower_id, size_t byte_offset);
#ifdef RAFT_TEST_MODE
 private:
#endif
  void TruncatePrefixLocked(uint64_t snapshot_index);
  void SendSnapshotsToBehindFollowersLocked();
  std::string ReadSnapshotFileData(std::string snapshot_file_path_);
  void SetRoleLocked(Role role);  // Must be called under mutex_.

  // Writes the current storage state machine snapshot to snapshot_file_path_
  // atomically via a temp file. Must NOT be called under mutex_.
  bool WriteSnapshotToDisk();
  // Reads one chunk of chunk_size_in_bytes_ from snapshot_file_path_ starting
  // at byte_offset. Fills chunk_out, total_size_out, and done_out. Returns
  // false on any I/O failure. Must NOT be called under mutex_.
  bool ReadSnapshotChunk(size_t byte_offset, std::string& chunk_out,
                         size_t& total_size_out, bool& done_out);
  // Applies a fully-received snapshot: resets log state, renames the temp file,
  // reads the snapshot back, and calls ApplySnapshot + WriteMetadataLocked.
  // Must NOT be called under mutex_.
  void InstallReceivedSnapshot(uint64_t last_included_index,
                               uint64_t last_included_term,
                               const std::string& tmp_path);

  // Persistent state on all servers:
  uint64_t current_term_;      // Protected by mutex_
  int voted_for_;              // Protected by mutex_
  std::vector<LogEntry> log_;  // Protected by mutex_

  // Volatile state on leaders:
  std::vector<FollowerProgress> progress_;  // Protected by mutex_
  uint64_t heartbeats_sent_this_term_;  // Protected by mutex_

  // Volatile state on all servers:
  uint64_t last_log_index_;  // Protected by mutex_
  uint64_t commit_index_;  // Protected by mutex_
  // last_committed stores the last entry that has been passed to commit_, but
  // it may not yet have been executed. Raft's Consensus file holds
  // last_applied_
  uint64_t last_committed_;  // Protected by mutex_
  Role role_;                // Protected by mutex_
  int current_leader_;       // Protected by mutex_
  std::vector<int> votes_;                                // Protected by mutex_
  // These are required by the Raft algorithm, but only used when actually
  // sending the snapshot and in recording the metadata.
  uint64_t snapshot_last_index_, snapshot_last_term_;  // Protected by mutex_
  // Since we leave some amount of buffer for snapshotted terms before
  // truncation, this is used for all log arithmetic and to see if an entry is
  // contained in our log or not.
  uint64_t truncated_last_index_, truncated_last_term_;
  // Drain the snapshot queue
  bool drain_requested_;  // Protected by snapshot_queue_mutex_

  // Reassembly state for incoming chunked InstallSnapshot RPCs (follower side).
  // Key: leader_id. Cleared when snapshot finishes or a new one starts.
  // Chunks are written to a temp file as they arrive. On completion the temp
  // file is renamed to the final snapshot path and applied to the state
  // machine.
  struct PendingSnapshot {
    uint64_t last_included_index = 0;
    uint64_t last_included_term = 0;
    // Next byte offset we expect from the leader.
    uint64_t expected_offset = 0;
    // Open fd to the temp file, or -1 if not open.
    int fd = -1;
    std::string tmp_path;
  };
  std::map<int, PendingSnapshot>
      pending_snapshot_chunks_;  // Protected by mutex_
  // 1 MiB per snapshot chunk
  static constexpr size_t chunk_size_in_bytes_ = 1 * 1024 * 1024;

  // final committed snapshot path
  std::string snapshot_file_path_;
  // temp path used during leader serialization
  std::string snapshot_tmp_path_;

  bool is_stop_;
  const uint64_t quorum_;

  static constexpr size_t max_header_bytes_ = 64;
  // for limiting AppendEntries batch sizing
  static constexpr size_t max_bytes_ = 64 * 1024 * 16 * 16;
  static constexpr size_t max_entries_ = 128 * 10; /*128;*/
  size_t max_in_flight_per_follower_ = 128;
  // Set this to true to ignore the above restrictions on messages sent to
  // followers.
  bool do_not_limit_in_flight_messages_ = false;
  static constexpr std::chrono::milliseconds ae_response_deadline_{
      1500000};  // in milliseconds
  std::chrono::steady_clock::time_point
      timestamp_since_last_transaction_batch_ =
          std::chrono::steady_clock::now();
  std::chrono::milliseconds batch_threshold_{5};
  bool enable_batching_ = true;
  // This is the number of entries that are covered by the snapshot that will
  // remain in the log.
  uint64_t snapshot_buffer_amount_ = 5000;
  static constexpr std::chrono::seconds snapshot_response_deadline_{30};

  SignatureVerifier* verifier_;
  LeaderElectionManager* leader_election_manager_;
  ReplicaCommunicator* replica_communicator_;
  RaftRecovery* recovery_;
  ResDBConfig config_;
  std::thread snapshot_sending_thread_;
  LockFreeQueue<std::pair<uint64_t, size_t>> snapshot_queue_;
  std::vector<std::pair<std::chrono::steady_clock::time_point, size_t>>
      snapshot_send_time_;

#ifdef RAFT_TEST_MODE
 public:
  void SetStateForTest(RaftStatePatch patch) {
    std::lock_guard lk(mutex_);
    if (patch.current_term) {
      current_term_ = *patch.current_term;
    }
    if (patch.voted_for) {
      voted_for_ = *patch.voted_for;
    }
    if (patch.commit_index) {
      commit_index_ = *patch.commit_index;
    }
    if (patch.last_committed) {
      last_committed_ = *patch.last_committed;
    }
    if (patch.role) {
      role_ = *patch.role;
    }

    if (patch.snapshot_last_index) {
      snapshot_last_index_ = *patch.snapshot_last_index;
    }
    if (patch.snapshot_last_term) {
      snapshot_last_term_ = *patch.snapshot_last_term;
    }
    if (patch.truncated_last_index) {
      truncated_last_index_ = *patch.truncated_last_index;
    }
    if (patch.truncated_last_term) {
      truncated_last_term_ = *patch.truncated_last_term;
    }

    if (patch.log) {
      log_ = *patch.log;
      last_log_index_ = log_.size() - 1 + truncated_last_index_;
    }

    if (patch.progress) {
      CHECK_EQ(progress_.size(), patch.progress->size());

      for (size_t i = 0; i < patch.progress->size(); ++i) {
        const FollowerProgressPatch& progress_patch = (*patch.progress)[i];

        FollowerProgress& progress = progress_[i];

        if (progress_patch.state) {
          progress.state = *progress_patch.state;
        }

        if (progress_patch.match_index) {
          progress.match_index = *progress_patch.match_index;
        }

        if (progress_patch.next_index) {
          progress.next_index = *progress_patch.next_index;
        }

        if (progress_patch.in_flight) {
          progress.in_flight = *progress_patch.in_flight;
        }
      }
    }
    if (patch.votes) {
      votes_ = *patch.votes;
    }
    if (patch.enable_batching) {
      enable_batching_ = *patch.enable_batching;
    }
    if (patch.snapshot_buffer_amount) {
      snapshot_buffer_amount_ = *patch.snapshot_buffer_amount;
    }
    if (patch.max_in_flight_per_follower) {
      max_in_flight_per_follower_ = *patch.max_in_flight_per_follower;
    }
  }

  uint64_t GetTruncatedLastIndex() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return truncated_last_index_;
  }

  uint64_t GetCurrentTerm() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return current_term_;
  }

  int GetVotedFor() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return voted_for_;
  }

  const std::vector<LogEntry>& GetLog() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return log_;
  }

  void PrintLog(std::ostream& os) const {
    os << "Log entries (count = " << log_.size() << "):\n";

    for (size_t i = 0; i < log_.size(); ++i) {
      const auto& entry = log_[i];

      os << "  [" << i << "] "
         << "term=" << entry.entry.term() << ", command=\""
         << entry.entry.command() << "\""
         << ", serialized_size=" << entry.GetSerializedSize() << "\n";
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

  std::vector<size_t> GetNextIndex() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<size_t> result;
    result.reserve(progress_.size());
    for (const auto& progress : progress_) {
      result.push_back(progress.next_index);
    }
    return result;
  }

  std::vector<size_t> GetMatchIndex() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<size_t> result;
    result.reserve(progress_.size());
    for (const auto& progress : progress_) {
      result.push_back(progress.match_index);
    }
    return result;
  }

  std::vector<FollowerProgress> GetFollowerProgress() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return progress_;
  }

  uint64_t GetHeartbeatsSentThisTerm() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return heartbeats_sent_this_term_;
  }

  uint64_t GetLastLogIndex() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return last_log_index_;
  }

  uint64_t GetCommitIndex() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return commit_index_;
  }

  uint64_t GetLastCommitted() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return last_committed_;
  }

  Role GetRole() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return role_;
  }

  std::vector<int> GetVotes() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return votes_;
  }

  size_t GetMaxInFlightVecs() const { return max_in_flight_per_follower_; }

  std::chrono::milliseconds GetAEResponseDeadline() const {
    return ae_response_deadline_;
  }

#endif
};

}  // namespace raft
}  // namespace resdb
