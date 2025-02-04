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

#include "platform/common/queue/batch_queue.h"
#include "platform/config/resdb_config.h"
#include "platform/consensus/ordering/pbft/message_manager.h"
#include "platform/consensus/ordering/pbft/response_manager.h"
#include "platform/networkstrate/replica_communicator.h"
#include "platform/statistic/stats.h"

namespace resdb {

class Commitment {
 public:
  Commitment(const ResDBConfig& config, MessageManager* message_manager,
             ReplicaCommunicator* replica_communicator,
             SignatureVerifier* verifier);
  virtual ~Commitment();

  virtual int ProcessNewRequest(std::unique_ptr<Context> context,
                                std::unique_ptr<Request> user_request);

  virtual int ProcessProposeMsg(std::unique_ptr<Context> context,
                                std::unique_ptr<Request> request);
  virtual int ProcessPrepareMsg(std::unique_ptr<Context> context,
                                std::unique_ptr<Request> request);
  virtual int ProcessCommitMsg(std::unique_ptr<Context> context,
                               std::unique_ptr<Request> request);

  void SetPreVerifyFunc(std::function<bool(const Request& request)> func);
  void SetNeedCommitQC(bool need_qc);

 protected:
  virtual int PostProcessExecutedMsg();

 protected:
  ResDBConfig config_;
  MessageManager* message_manager_;
  std::thread executed_thread_;
  std::atomic<bool> stop_;
  ReplicaCommunicator* replica_communicator_;

  SignatureVerifier* verifier_;
  Stats* global_stats_;

  std::function<bool(const Request& request)> pre_verify_func_;
  bool need_qc_ = false;
};

}  // namespace resdb
