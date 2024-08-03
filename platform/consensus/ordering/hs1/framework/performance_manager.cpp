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

#include "platform/consensus/ordering/hs1/framework/performance_manager.h"

#include <glog/logging.h>

#include "common/utils/utils.h"

namespace resdb {
namespace hs1 {

using comm::CollectorResultCode;

HotStuff1PerformanceManager::HotStuff1PerformanceManager(
    const ResDBConfig& config, ReplicaCommunicator* replica_communicator,
    SignatureVerifier* verifier)
    : PerformanceManager(config, replica_communicator, verifier){
  client_num_ = 1;
  primary_ = id_ % replica_num_;
}


CollectorResultCode HotStuff1PerformanceManager::AddResponseMsg(
    std::unique_ptr<Request> request,
    std::function<void(std::unique_ptr<BatchUserResponse>)> response_call_back) {
  if (request == nullptr) {
    return CollectorResultCode::INVALID;
  }

  //uint64_t seq = request->seq();

  std::unique_ptr<BatchUserResponse> batch_response = std::make_unique<BatchUserResponse>();
  if (!batch_response->ParseFromString(request->data())) {
    LOG(ERROR) << "parse response fail:"<<request->data().size()
    <<" seq:"<<request->seq(); return CollectorResultCode::INVALID;
  }

  uint64_t seq = batch_response->local_id();
  //LOG(ERROR)<<"receive seq:"<<seq;

  bool done = false;
  {
    int idx = seq % response_set_size_;
    std::unique_lock<std::mutex> lk(response_lock_[idx]);
    if (response_[idx].find(seq) == response_[idx].end()) {
      //LOG(ERROR)<<"has done local seq:"<<seq<<" global seq:"<<request->seq();
      return CollectorResultCode::OK;
    }
    response_[idx][seq]++;
    //LOG(ERROR)<<"get seq :"<<request->seq()<<" local id:"<<seq<<" num:"<<response_[idx][seq]<<" send:"<<send_num_;
    if (response_[idx][seq] >= config_.GetMinDataReceiveNum()) {
      //LOG(ERROR)<<"get seq :"<<request->seq()<<" local id:"<<seq<<" num:"<<response_[idx][seq]<<" done:"<<send_num_;
      response_[idx].erase(response_[idx].find(seq));
      done = true;
    }
  }
  if (done) {
    response_call_back(std::move(batch_response));
    return CollectorResultCode::STATE_CHANGED;
  }
  return CollectorResultCode::OK;
}

int HotStuff1PerformanceManager::GetPrimary(){
  int view, value;
  while (true) {
    view = primary_;
    // LOG(ERROR) << "Dakai";
    primary_ += client_num_;
    value = view % replica_num_ + 1;
    if (value % 3 == 1 && value < 3 * config_.GetNonResponsiveNum()) {
      continue;
    }
    if (value % 3 == 1 && value < 3 * config_.GetForkTailNum()) {
      send_num_--;
    }
    // LOG(ERROR) << "Send to "<< value << " with send_num_: " << send_num_; 
    return value;
  } 
}

}  // namespace hs1
}  // namespace resdb