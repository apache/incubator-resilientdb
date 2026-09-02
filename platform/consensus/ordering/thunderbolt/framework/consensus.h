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

#include "executor/common/transaction_manager.h"
#include "platform/consensus/execution/transaction_executor.h"
#include "platform/consensus/ordering/common/framework/consensus.h"
#include "platform/consensus/ordering/thunderbolt/protocol/thunderbolt.h"
#include "platform/networkstrate/consensus_manager.h"

namespace resdb {
namespace thunderbolt {

class ThunderboltConsensus : public common::Consensus {
 public:
  ThunderboltConsensus(const ResDBConfig& config,
                std::unique_ptr<TransactionManager> transaction_manager);

  void SetupPerformancePreprocessFunc(
      std::function<std::vector<std::string>()> func);

  void SetVerifiyFunc(std::function<bool(const Request&)> func);

 protected:
  int ProcessCustomConsensus(std::unique_ptr<Request> request) override;
  int ProcessNewTransaction(std::unique_ptr<Request> request) override;
  int CommitMsg(const google::protobuf::Message& msg) override;
  int CommitMsgInternal(const Transaction& txn);

  void AsyncPreprocess();
  void SetPreprocessFunc(std::function<int(Request*)> func);
  void Preprocess(std::unique_ptr<Request> request);

  bool VerifyTransaction(const Transaction& txn);
  void StopCallBack(int proposer);
  void Process2PC(std::unique_ptr<Request> req);
  void ReceiveTPC(std::unique_ptr<TPCRequest> req);
  void ReceiveTPCACK(std::unique_ptr<TPCRequest> req);
  void ReceiveTPC2(std::unique_ptr<TPCRequest> req);
  void CommitTPC(std::unique_ptr<TPCRequest> req);

  void ReleaseLock(const TPCRequest& req);
  int AddLock(const TPCRequest& req);

 private:
  std::unique_ptr<Thunderbolt> thunderbolt_;
  std::thread preprocess_thread_;
  std::function<int(Request*)> preprocess_func_;
  std::function<bool(const Request&)> verify_func_;
  LockFreeQueue<Request> pre_q_;
  std::set<int> stop_proposer_;
  std::map<std::string, std::unique_ptr<Request> > pc_data_;
  std::map<int, std::set<int> > tpc_lock_;
  std::map<int, std::set<int> > tpc_received_;
  std::mutex lock_mutex_;
  int f_;
  int id_;
};

}  // namespace thunderbolt
}  // namespace resdb
