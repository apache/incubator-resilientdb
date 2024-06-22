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

#include "platform/config/resdb_config.h"
#include "platform/consensus/ordering/poe/nomac/message_manager.h"
#include "platform/consensus/ordering/poe/proto/poe.pb.h"
#include "platform/networkstrate/replica_communicator.h"
#include "platform/statistic/stats.h"

namespace resdb {
namespace poe {

class Commitment {
 public:
  Commitment(
      const ResDBConfig& config, MessageManager* message_manager,
      ReplicaCommunicator* replica_communicator, SignatureVerifier* verifier,
      std::function<int(int, const ::google::protobuf::Message& message, int)>
          client_call);
  virtual ~Commitment();

  int ProcessNewTransaction(std::unique_ptr<Request> request);
  int Process(int type, std::unique_ptr<POERequest> request);
  void SetNodeId(const int32_t id);
  int ProcessNewRequest(std::unique_ptr<POERequest> request);
  int ProcessViewChange(std::unique_ptr<ViewChangeMsg> vc_msg);

 protected:
  virtual int PostProcessExecutedMsg();
  POERequest::Type GetNextState(int type);
  POERequest::Type VoteType(int type);
  std::unique_ptr<Request> NewRequest(
      const POERequest& request, POERequest::Type type = POERequest::TYPE_NONE);
  int ProcessMessageOnPrimary(std::unique_ptr<POERequest> request);
  int ProcessMessageOnReplica(std::unique_ptr<POERequest> request);
  int ProcessNewView(int64_t view_number);

  void SendFailRepsonse(const Request& user_request);
  void RedirectUserRequest(const Request& user_request);

  void StartNewView(std::unique_ptr<Request> user_request);
  int32_t PrimaryId(int64_t view_num);
  QC GetQC(int64_t seq);

  bool VerifyNodeSignagure(const POERequest& request);
  bool VerifyTS(const POERequest& request);
  bool VerifyTxn(const POERequest& request);

  std::unique_ptr<POERequest> GetNewViewMessage(int64_t view_number);

 protected:
  ResDBConfig config_;
  MessageManager* message_manager_;
  std::thread executed_thread_;
  std::atomic<bool> stop_;
  // ReplicaCommunicator* replica_communicator_;
  SignatureVerifier* verifier_;
  int32_t id_;
  std::map<int64_t, std::set<uint64_t>> received_senders_;
  std::map<int64_t, std::vector<std::unique_ptr<POERequest>>>
      received_requests_;
  std::function<int(int, const ::google::protobuf::Message& message, int)>
      client_call_;

  std::mutex mutex_, xmutex_;
  Stats* global_stats_ = nullptr;
};

}  // namespace poe
}  // namespace resdb
