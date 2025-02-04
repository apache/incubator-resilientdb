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

#include "executor/common/transaction_manager.h"
#include "platform/consensus/execution/transaction_executor.h"
#include "platform/consensus/ordering/common/framework/consensus.h"
#include "platform/consensus/ordering/tusk/protocol/tusk.h"
#include "platform/networkstrate/consensus_manager.h"

namespace resdb {
namespace tusk {

class TuskConsensus : public common::Consensus {
 public:
  TuskConsensus(const ResDBConfig& config,
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
  std::unique_ptr<Tusk> tusk_;
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

}  // namespace tusk
}  // namespace resdb
