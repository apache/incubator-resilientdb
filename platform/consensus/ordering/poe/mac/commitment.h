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
#include "platform/consensus/ordering/poe/mac/message_manager.h"
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

 private:
  int ProcessNewRequest(std::unique_ptr<POERequest> request);
  int ProcessSupport(std::unique_ptr<POERequest> request);
  int ProcessCeritify(std::unique_ptr<POERequest> request);

 protected:
  virtual int PostProcessExecutedMsg();
  Request::Type GetNextState(int type);
  std::unique_ptr<Request> NewRequest(const Request& request,
                                      Request::Type type = Request::TYPE_NONE);

  std::unique_ptr<Request> NewRequest(const POERequest& request,
                                      POERequest::Type type);
  int ProcessNewView(int64_t view_number);

  void SendFailRepsonse(const Request& client_request);
  void RedirectUserRequest(const Request& client_request);

  void StartNewView(std::unique_ptr<Request> client_request);
  int32_t PrimaryId(int64_t view_num);

  std::unique_ptr<Request> GetNewViewMessage(int64_t view_number);

 protected:
  ResDBConfig config_;
  MessageManager* message_manager_;
  std::thread executed_thread_;
  std::atomic<bool> stop_;
  // ReplicaCommunicator* replica_communicator_;
  SignatureVerifier* verifier_;
  // std::atomic<uint64_t> current_seq_;
  int32_t id_;
  std::function<int(int, const ::google::protobuf::Message& message, int)>
      client_call_;

  std::mutex mutex_;
  Stats* global_stats_ = nullptr;
};

}  // namespace poe
}  // namespace resdb
