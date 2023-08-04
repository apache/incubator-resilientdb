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

#include "platform/consensus/ordering/common/commitment_basic.h"
#include "platform/consensus/ordering/hotstuff/message_manager.h"

namespace resdb {
namespace hotstuff {

class Commitment : public CommitmentBasic {
 public:
  Commitment(const ResDBConfig& config, MessageManager* message_manager,
             ReplicaCommunicator* replica_communicator,
             SignatureVerifier* verifier);
  virtual ~Commitment();

  int Process(std::unique_ptr<HotStuffRequest> request);
  void Init();
  int ProcessNewRequest(std::unique_ptr<HotStuffRequest> request);

 protected:
  HotStuffRequest::Type GetNextState(int type);
  HotStuffRequest::Type VoteType(int type);
  std::unique_ptr<Request> NewRequest(
      const HotStuffRequest& request,
      HotStuffRequest::Type type = HotStuffRequest::TYPE_NONE);
  int ProcessMessageOnPrimary(std::unique_ptr<HotStuffRequest> request);
  int ProcessMessageOnReplica(std::unique_ptr<HotStuffRequest> request);
  QC GetQC(int64_t view_num, HotStuffRequest::Type type);
  int ProcessNewView(int64_t view_number);
  void SendNewView();
  std::unique_ptr<HotStuffRequest> GetClientRequest();
  QC GetHighQC(int64_t view_num);

  bool VerifyNodeSignagure(const HotStuffRequest& request);
  bool VerifyTS(const HotStuffRequest& request);

  std::unique_ptr<HotStuffRequest> GetNewViewMessage(int64_t view_number);

 protected:
  MessageManager* message_manager_;
  HotStuffRequest::Type current_state_;
  QC lockQC_;
  int64_t current_view_;
  std::map<HotStuffRequest::Type, std::set<uint64_t>> received_senders_[128];
  std::map<HotStuffRequest::Type, std::vector<std::unique_ptr<HotStuffRequest>>>
      received_requests_[128];

  LockFreeQueue<HotStuffRequest> request_list_;
  std::mutex mutex_[129];
};

}  // namespace hotstuff
}  // namespace resdb
