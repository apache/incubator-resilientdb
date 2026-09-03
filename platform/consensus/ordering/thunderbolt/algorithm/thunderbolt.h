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

#include "common/crypto/signature_verifier.h"
#include "platform/common/queue/lock_free_queue.h"
#include "platform/consensus/ordering/common/algorithm/protocol_base.h"
#include "platform/consensus/ordering/thunderbolt/algorithm/proposal_manager.h"
#include "platform/consensus/ordering/thunderbolt/proto/proposal.pb.h"
#include "platform/statistic/stats.h"

namespace resdb {
namespace thunderbolt {

class Thunderbolt : public common::ProtocolBase {
 public:
  Thunderbolt(int id, int f, int total_num, SignatureVerifier* verifier);
  ~Thunderbolt();

  bool ReceiveTransaction(std::unique_ptr<Transaction> txn);
  bool ReceiveBlock(std::unique_ptr<Proposal> proposal);
  void ReceiveBlockACK(std::unique_ptr<Metadata> metadata);
  void ReceiveBlockCert(std::unique_ptr<Certificate> cert);

 private:
  void CommitProposal(int round, int proposer);
  void CommitRound(int round);
  void AsyncCommit();
  void AsyncSend();
  void AsyncExecute();

  bool VerifyCert(const Certificate& cert);

  bool CheckBlock(const Proposal& p);
  void CheckFutureBlock(int round);
  void CheckFutureCert(const Proposal& proposal);
  void CheckFutureCert(int round, const std::string& hash, int proposer);
  bool CheckCert(const Certificate& cert);
  bool SendBlockAck(std::unique_ptr<Proposal> proposal);

  void AsyncProcessCert();

  int GetLeader(int64_t r);

 private:
  LockFreeQueue<Proposal> execute_queue_, pending_block_;
  LockFreeQueue<int> commit_queue_;
  LockFreeQueue<Transaction> txns_;

  std::unique_ptr<ProposalManager> proposal_manager_;
  SignatureVerifier* verifier_;

  std::thread send_thread_;
  std::thread commit_thread_, execute_thread_;
  std::mutex txn_mutex_, mutex_;
  int limit_count_;
  std::map<std::string, std::map<int, std::unique_ptr<Metadata>>> received_num_;
  std::condition_variable vote_cv_;
  int start_ = 0;
  int batch_size_ = 0;
  int execute_id_ = 1;
  std::atomic<int> queue_size_;

  Stats* global_stats_;

  std::thread cert_thread_;
  LockFreeQueue<Certificate> cert_queue_;
  std::mutex future_block_mutex_, future_cert_mutex_, check_block_mutex_;
  std::map<int, std::map<std::string, std::unique_ptr<Proposal>>> future_block_;
  std::map<int, std::map<std::string, std::unique_ptr<Certificate>>>
      future_cert_;
  int64_t last_commit_time_ = 0;
};

}  // namespace thunderbolt
}  // namespace resdb
