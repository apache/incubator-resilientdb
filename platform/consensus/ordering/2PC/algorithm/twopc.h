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

#include <google/protobuf/message.h>

#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include "common/crypto/signature_verifier.h"
#include "platform/consensus/ordering/common/algorithm/protocol_base.h"
#include "platform/proto/resdb.pb.h"

namespace resdb {
namespace twopc {

class TwoPC : public common::ProtocolBase {
 public:
  enum MsgType {
    PREPARE = 1,
    VOTE = 2,
    COMMIT = 3,
    ABORT = 4,
  };

  TwoPC(int id, int f, int total_num);

  void SetCoordinatorId(int id) { coordinator_id_ = id; }

  // Coordinator methods
  int StartTransaction(const std::string& transaction_id, const Request& request);
  int ProcessVote(const std::string& transaction_id, int node_id, bool vote);

  // Participant methods
  int ProcessPrepare(const std::string& transaction_id, const Request& request);
  bool ValidateTransaction(const Request& request);
  int SendVote(const std::string& transaction_id, bool vote);

  // Decision handling (called on all nodes when COMMIT/ABORT received)
  int ProcessDecision(const std::string& transaction_id, bool commit);

 private:
  int coordinator_id_ = 0;
  std::mutex mutex_;
  std::map<std::string, Request> pending_transactions_;
  std::map<std::string, std::vector<bool>> transaction_votes_;
  std::map<std::string, bool> transaction_decisions_;
};

}  // namespace twopc
}  // namespace resdb