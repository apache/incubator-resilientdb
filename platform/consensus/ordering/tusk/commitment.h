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

#pragma once

#include <queue>

#include "platform/consensus/ordering/common/commitment_basic.h"
#include "platform/consensus/ordering/tusk/message_manager.h"

namespace resdb {
namespace tusk {

class Commitment : public CommitmentBasic {
 public:
  Commitment(const ResDBConfig& config, MessageManager* message_manager,
             ReplicaCommunicator* replica_client, SignatureVerifier* verifier);
  virtual ~Commitment();

  int Process(std::unique_ptr<TuskRequest> request);
  int ProcessNewRequest(std::unique_ptr<TuskRequest> request);
  int ProcessMetadata(std::unique_ptr<TuskBlockMetadata> metadata);
  int ProcessRequestPropose(std::unique_ptr<TuskRequest> request);

  void Init();

 protected:
  void DAGConsensus();
  void ProcessCommit(std::unique_ptr<TuskRequest> leader_req,
                     int64_t previous_round);

  void BroadCastBlock(const TuskRequest& block, TuskRequest::Type type);
  void SendBlock(const TuskRequest& block, TuskRequest::Type type);
  void BroadCastMetadata(const TuskBlockMetadata& metadata,
                         TuskRequest::Type type);

  bool MayBroadCastBlock();
  void GenerateMetadata(const TuskRequest& new_block);

  void AddNewRequest(std::unique_ptr<TuskRequest> request);
  void AddReadyRequest(std::unique_ptr<TuskRequest> request);
  void AddBlockACK(std::unique_ptr<TuskRequest> request);

  bool CheckQC(const TuskRequest& request);
  void Sign(TuskRequest* request);
  bool Verify(const TuskRequest& request);

  void AddLeaderRound(int64_t round);
  int64_t GetLeaderRound();
  int GetLeader(int64_t r);

  std::unique_ptr<TuskRequest> GetRequest(int64_t round, int id);
  int GetReferenceNum(const TuskRequest& req);
  void RemoveData(int64_t round, int id);

 protected:
  MessageManager* message_manager_;
  std::thread executed_thread_, consensus_thread_;
  int64_t current_seq_, current_round_;

  std::map<std::string, std::unique_ptr<TuskRequest>> candidate_;
  std::queue<std::unique_ptr<TuskRequest>> new_request_;

  std::map<std::string, std::map<int64_t, std::unique_ptr<TuskRequest>>>
      received_block_ack_;

  std::map<int64_t, std::map<int, std::unique_ptr<TuskRequest>>>
      ready_request_list_;
  std::map<int64_t, std::map<int, std::unique_ptr<TuskBlockMetadata>>>
      metadata_list_;
  std::map<int, std::unique_ptr<TuskBlockMetadata>>
      latest_metadata_from_sender_;

  std::map<int64_t, std::map<int64_t, std::set<int>>> reference_;

  std::queue<int64_t> leader_round_;

  std::mutex mutex_;

  std::condition_variable ld_cv_;
  std::mutex ld_mutex_;
};

}  // namespace tusk
}  // namespace resdb
