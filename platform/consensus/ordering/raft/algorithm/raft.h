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

#include <deque>
#include <map>
#include <queue>
#include <thread>

#include "platform/common/queue/lock_free_queue.h"
#include "platform/consensus/ordering/common/algorithm/protocol_base.h"
#include "platform/consensus/ordering/raft/proto/proposal.pb.h"
#include "platform/statistic/stats.h"

namespace resdb {
namespace raft {

class Raft : public common::ProtocolBase {
 public:
  Raft(int id, int f, int total_num, SignatureVerifier* verifier);
  ~Raft();

  bool ReceiveTransaction(std::unique_ptr<AppendEntries> txn);
  bool ReceivePropose(std::unique_ptr<AppendEntries> txn);
  bool ReceiveAppendEntriesResponse(std::unique_ptr<AppendEntriesResponse> response);
 private:
  bool IsStop();
  void Dump();

 private:
  std::mutex mutex_;
  std::map<std::string, std::set<int32_t> > received_;
  std::map<std::string, std::unique_ptr<AppendEntries> > data_; // log[]

  std::vector<std::string> dataIndexMapping_;

  // This is for everyone
  // Most recent term it has seen
  int currentTerm_;
  // Id for vote in current Term
  int votedFor_;

  // Volatile on all servers
  // Index of highest log entry it knows to be committed
  int64_t commitIndex_;
  // Index of highest log entry executed
  int64_t lastApplied_;

  // Only for leaders
  // This keeps track of the next log entry to send to that replica
  // Initialized to last log index + 1
  std::vector<int> nextIndex_;
  // This keeps track of the highest log entry it knows is executed on that replica
  std::vector<int> matchIndex_;

  int64_t seq_;
  bool is_stop_;
  SignatureVerifier* verifier_;
  Stats* global_stats_;
};

}  // namespace raft
}  // namespace resdb
