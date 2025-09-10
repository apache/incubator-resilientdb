/*

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

  bool first = false;
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

    if (response_[idx][seq] == 1) {
      first = true;
    }

    if (response_[idx][seq] >= config_.GetMinDataReceiveNum()) {
      //LOG(ERROR)<<"get seq :"<<request->seq()<<" local id:"<<seq<<" num:"<<response_[idx][seq]<<" done:"<<send_num_;
      response_[idx].erase(response_[idx].find(seq));
      done = true;
    }
  }

  if (first) {
    response_call_back(std::move(batch_response));
    return CollectorResultCode::FIRST_RESPONSE;
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
    primary_ += client_num_;
    value = view % replica_num_ + 1;
    if (value % 3 == 1 && value < 3 * crash_num_) {
      send_num_ --;
    }
    if (crash_num_ > 0) {
      if (value == 3 * crash_num_ + 1) {
        send_num_ ++;
      }
      if (value == replica_num_) {
        send_num_ --;
      }
    }

    if (value % 3 == 1 && value < 3 * config_.GetForkTailNum()) {
      send_num_--;
    }
    if (value % 3 == 1 && value < 3 * config_.GetRollBackNum()) {
      send_num_--;
    }
    LOG(ERROR) << "Send to "<< value << " with send_num_: " << send_num_; 
    return value;
  } 
}

// =================== response ========================
// handle the response message. If receive f+1 commit messages, send back to the
// user.
int HotStuff1PerformanceManager::ProcessResponseMsg(std::unique_ptr<Context> context,
                                           std::unique_ptr<Request> request) {
  std::unique_ptr<Request> response;
  // Add the response message, and use the call back to collect the received
  // messages.
  // The callback will be triggered if it received f+1 messages.
  if (request->ret() == -2) {
    // LOG(INFO) << "get response fail:" << request->ret();
    send_num_--;
    return 0;
  }

  LOG(INFO) << "get response:" << request->seq() << " sender:"<<request->sender_id();
  std::unique_ptr<BatchUserResponse> batch_response = nullptr;
  CollectorResultCode ret =
      AddResponseMsg(std::move(request), [&](std::unique_ptr<BatchUserResponse> request) {
        batch_response = std::move(request);
        return;
      });

  if (config_.NetworkDelayNum() > 0) {
    if (ret == CollectorResultCode::FIRST_RESPONSE) {
      assert(batch_response);
      int primary_id = batch_response->primary_id();
      // LOG(ERROR) << "here: " << primary_id;
      inflight_[primary_id]--;
      last_response_ = primary_id;
      send_num_--;

    } 
    if (ret == CollectorResultCode::STATE_CHANGED) {
      assert(batch_response);
      SendResponseToClient(*batch_response);
    }
  }
  else {
    if (ret == CollectorResultCode::STATE_CHANGED) {
      assert(batch_response);
      int primary_id = batch_response->primary_id();
      LOG(ERROR) << "primary_id: " << primary_id;
      inflight_[primary_id]--;
      last_response_ = primary_id;
      SendResponseToClient(*batch_response);
    }
  }
  return ret == CollectorResultCode::INVALID ? -2 : 0;
}

}  // namespace hs1
}  // namespace resdb