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

#include "platform/common/queue/batch_queue.h"
#include "platform/config/resdb_config.h"
#include "platform/consensus/execution/duplicate_manager.h"
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

  std::queue<std::pair<std::unique_ptr<Context>, std::unique_ptr<Request>>>
      request_complained_;

  std::mutex rc_mutex_;

  DuplicateManager* GetDuplicateManager();

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

  std::mutex mutex_;
  std::unique_ptr<DuplicateManager> duplicate_manager_;
};

}  // namespace resdb
