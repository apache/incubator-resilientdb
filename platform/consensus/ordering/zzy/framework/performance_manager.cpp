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

#include "platform/consensus/ordering/zzy/framework/performance_manager.h"

#include <glog/logging.h>

#include "common/utils/utils.h"

namespace resdb {
namespace zzy {

using comm::CollectorResultCode;

int ZZYPerformanceManager::Broadcast(int type, const google::protobuf::Message& msg) {
  Request request;
  msg.SerializeToString(request.mutable_data());
  request.set_type(Request::TYPE_CUSTOM_CONSENSUS);
  request.set_user_type(type);
  request.set_sender_id(config_.GetSelfInfo().id());

  replica_communicator_->BroadCast(request);
  return 0;
}

ZZYPerformanceManager::ZZYPerformanceManager(
    const ResDBConfig& config, ReplicaCommunicator* replica_communicator,
    SignatureVerifier* verifier)
    : PerformanceManager(config, replica_communicator, verifier){
  
      total_replicas_ = config_.GetReplicaNum();
      f_ = (total_replicas_ - 1) / 3;
  start_seq_ = 1;
  time_limit_ = 5000;
  ready_thread_ = std::thread(&ZZYPerformanceManager::WaitComplete, this);
}

void ZZYPerformanceManager::AddPkg(int64_t local_id, std::unique_ptr<BatchUserResponse> resp) {
      std::unique_lock<std::mutex> lk(n_mutex_);
  if(done_.find(local_id) != done_.end() || local_id < start_seq_){
    return;
  }
  done_[local_id] = GetCurrentTime();
  std::unique_lock<std::mutex> rlk(resp_mutex_[local_id%1000]);
  resp_[local_id%1000][local_id] = std::move(resp);
  //LOG(ERROR)<<" add resp:"<<local_id;
}

void ZZYPerformanceManager::PkgDone(int local_id) {
  if(local_id < start_seq_) {
    return;
  }
  done_[local_id] = 0;
}

bool ZZYPerformanceManager::IsTimeout() {
  if(done_.begin()->second == 0) {
    return false;
  }
  return GetCurrentTime() - done_.begin()->second > time_limit_;
}

bool ZZYPerformanceManager::TimeoutLeft() {
  return time_limit_ - GetCurrentTime() - done_.begin()->second > time_limit_;
}



bool ZZYPerformanceManager::Ready() {
  if (done_.empty()) {
    // LOG(ERROR) << "empty";
  } else {
    LOG(ERROR) << "done begin " <<done_.begin()->first << " start_seq_: " << start_seq_;
  }
  if(done_.empty() || done_.begin()->first != start_seq_){
    return false;
  }
  if(done_.begin()->second == 0 || IsTimeout()){
    return true;
  }
  return false;
}

int ZZYPerformanceManager::CheckReady() {
  while(Ready()){
    if(IsTimeout()){
      LOG(ERROR)<<" check timeout seq:"<<start_seq_<<" start:"<<done_.begin()->second<<" end:"<<GetCurrentTime() <<" diff:"<<GetCurrentTime() - done_.begin()->second;
      SendTimeout(start_seq_);
    }
    done_.erase(done_.begin());
    start_seq_++;
  }
  return 0;
}

void ZZYPerformanceManager::WaitComplete() {
  while (!stop_) {
      {
      std::unique_lock<std::mutex> lk(n_mutex_);
      vote_cv_.wait_for(lk, std::chrono::microseconds(1000),
          [&] { return Ready(); });
      CheckReady();
      }
      usleep(1000);
  }
}

void ZZYPerformanceManager::Notify(int seq) {
  int id = seq%1000;

  std::unique_ptr<BatchUserResponse> resp = nullptr;
  {
    std::unique_lock<std::mutex> lk(resp_mutex_[id]);
    if(resp_[id].find(seq) == resp_[id].end()){
      LOG(ERROR)<<" notify seq has done:"<<seq;
      return;
    }

    assert(resp_[id].find(seq) != resp_[id].end());
    resp = std::move(resp_[id][seq]);
    resp_[id].erase(resp_[id].find(seq));
  }

  SendResponseToClient(*resp);

  std::unique_lock<std::mutex> lk(n_mutex_);
  if(seq >= start_seq_) {
    PkgDone(seq);
  }
  vote_cv_.notify_one();
  LOG(ERROR)<<" notify seq:"<<seq;
}

bool ZZYPerformanceManager::ReceiveCommitACK(std::unique_ptr<Proposal> proposal) {
  int64_t seq = proposal->seq();
  int proposer = proposal->proposer();
  int local_id = proposal->local_id();
  bool done = false;
  {
    std::unique_lock<std::mutex> lk(c_mutex_[seq%1000]);
    commit_receive_[seq%1000][seq].insert(proposer);
    // LOG(ERROR)<<" client receive commit ack seq:"<<seq<<" proposer:"<<proposer<<" size:"<<commit_receive_[seq].size()<<" total_replicas:"<<total_replicas_<<" local id:"<<local_id;
    if(static_cast<int>(commit_receive_[seq%1000][seq].size()) == 2*f_+1) {
        done = true;
    }
  }
  if(done){
    Notify(local_id);
  }
  return true;
}

int ZZYPerformanceManager::ProcessResponseMsg(std::unique_ptr<Context> context,
    std::unique_ptr<Request> request) {
    int seq = request->seq();
    int proposer = request->sender_id();

    std::unique_ptr<BatchUserResponse> batch_response = std::make_unique<BatchUserResponse>();
    if (!batch_response->ParseFromString(request->data())) {
      LOG(ERROR) << "parse response fail:"<<request->data().size()
        <<" seq:"<<request->seq(); return CollectorResultCode::INVALID;
    }

    uint64_t local_id = batch_response->local_id();
    AddPkg(local_id, std::move(batch_response));

    std::unique_ptr<Request> req = std::make_unique<Request>(*request);
    bool done = false;
    {
      std::unique_lock<std::mutex> lk(mutex_[local_id%1000]);
      client_receive_[local_id%1000][local_id].push_back(std::move(req));
      //LOG(ERROR)<<" client receive seq:"<<seq<<" local id:"<<local_id<<" proposer:"<<proposer<<" size:"<<client_receive_[local_id%1000][local_id].size()<<" total_replicas:"<<total_replicas_;
      //if(static_cast<int>(client_receive_[local_id%1000][local_id].size()) == 2*f_+1) {
      if(static_cast<int>(client_receive_[local_id%1000][local_id].size()) == total_replicas_) {
        done = true;
      }
    }
    if(done) {
      Notify(local_id);
    }

    return 0;

}

bool ZZYPerformanceManager::SendTimeout(int local_id) {
  //assert(1==0);
  Proposal timeout_proposal;
  std::unique_lock<std::mutex> lk(mutex_[local_id%1000]);
  std::string hash; 
  for(auto & req: client_receive_[local_id%1000][local_id]) {
    auto qc = timeout_proposal.mutable_qc() -> add_qc();
    assert(req != nullptr);
    qc->set_data_hash(req->hash());
    *qc->mutable_sign() = req->data_signature();
    qc->set_seq(req->seq());
    hash = req->hash();
  }

  assert(timeout_proposal.qc().qc_size()>0);
  int seq = timeout_proposal.qc().qc(0).seq();

  timeout_proposal.set_hash(hash);
  timeout_proposal.set_seq(seq);
  timeout_proposal.set_proposer(id_);
  timeout_proposal.set_local_id(local_id);

  Broadcast(MessageType::Commit, timeout_proposal);

  return true;
}

}  // namespace common
}  // namespace resdb
