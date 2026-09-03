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

#include "platform/consensus/ordering/thunderbolt/proto/proposal.pb.h"

namespace resdb {
namespace thunderbolt {

class ProposalManager {
 public:
  ProposalManager(int32_t id, int limit_count);

  std::unique_ptr<Proposal> GenerateProposal(
      const std::vector<std::unique_ptr<Transaction>>& txns);

  int CurrentRound();
  void AddLocalBlock(std::unique_ptr<Proposal> proposal);
  const Proposal* GetLocalBlock(const std::string& hash);
  std::unique_ptr<Proposal> FetchLocalBlock(const std::string& hash);

  void AddBlock(std::unique_ptr<Proposal> proposal);
  void AddCert(std::unique_ptr<Certificate> cert);
  bool Ready();

  const Proposal* GetRequest(int round, int sender);
  std::unique_ptr<Proposal> FetchRequest(int round, int sender);
  int GetReferenceNum(const Proposal& req);

  bool VerifyHash(const Proposal& proposal);

  bool CheckCert(int round, int sender);
  bool CheckBlock(const std::string& hash);

  void SetCommittedRound(int r);

 protected:
  void GetMetaData(Proposal* proposal);

 private:
  int32_t id_;
  int round_;
  int limit_count_;
  std::map<std::string, std::unique_ptr<Proposal>> block_, local_block_;

  std::map<int64_t, std::map<int, std::unique_ptr<Certificate>>> cert_list_;
  std::map<int, std::unique_ptr<Certificate>> latest_cert_from_sender_;

  std::mutex txn_mutex_, local_mutex_;
  std::map<std::pair<int, int>, int> reference_;

  std::atomic<int> committed_round_ = 0;
};

}  // namespace thunderbolt
}  // namespace resdb
