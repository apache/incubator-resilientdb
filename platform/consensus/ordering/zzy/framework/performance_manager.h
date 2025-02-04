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

#include <thread>

#include "platform/consensus/ordering/common/framework/performance_manager.h"
#include "platform/consensus/ordering/zzy/proto/proposal.pb.h"

namespace resdb {
namespace zzy {

class ZZYPerformanceManager : public common::PerformanceManager {
 public:
  ZZYPerformanceManager(const ResDBConfig& config,
                        ReplicaCommunicator* replica_communicator,
                        SignatureVerifier* verifier);

  bool ReceiveCommitACK(std::unique_ptr<Proposal> proposal);

  bool Ready();
  int CheckReady();
  void WaitComplete();

  void Notify(int seq);
  int ProcessResponseMsg(std::unique_ptr<Context> context,
                         std::unique_ptr<Request> request) override;
  int Broadcast(int type, const google::protobuf::Message& msg);
  bool SendTimeout(int local_id);

  void AddPkg(int64_t local_id, std::unique_ptr<BatchUserResponse> resp);
  void PkgDone(int local_id);
  bool IsTimeout();
  bool TimeoutLeft();

 private:
  std::thread ready_thread_;
  std::mutex n_mutex_, mutex_[1000], c_mutex_[1000], resp_mutex_[1000];

  std::map<int64_t, int64_t> done_;
  std::condition_variable vote_cv_;
  std::map<int, std::vector<std::unique_ptr<Request> > > client_receive_[1000];
  std::map<int, std::set<int> > commit_receive_[1000];
  std::map<int64_t, std::unique_ptr<BatchUserResponse> > resp_[1000];

  int f_;
  int start_seq_;
  int total_replicas_;
  int64_t time_limit_;
};

}  // namespace zzy
}  // namespace resdb
