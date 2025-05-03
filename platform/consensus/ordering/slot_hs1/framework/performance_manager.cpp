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

#include "platform/consensus/ordering/slot_hs1/framework/performance_manager.h"

#include <glog/logging.h>

#include "common/utils/utils.h"

namespace resdb {
namespace slot_hs1 {

using comm::CollectorResultCode;

SlotHotStuff1PerformanceManager::SlotHotStuff1PerformanceManager(
    const ResDBConfig& config, ReplicaCommunicator* replica_communicator,
    SignatureVerifier* verifier)
    : PerformanceManager(config, replica_communicator, verifier){
  client_num_ = 1;
  primary_ = 2;
  slot_num_ = config_.GetSlotNum();
  last_response_ = 2;
  inflight_limit_ = 20;
}


CollectorResultCode SlotHotStuff1PerformanceManager::AddResponseMsg(
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


int SlotHotStuff1PerformanceManager::ProcessResponseMsg(std::unique_ptr<Context> context,
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

  //LOG(INFO) << "get response:" << request->seq() << " sender:"<<request->sender_id();
  std::unique_ptr<BatchUserResponse> batch_response = nullptr;
  CollectorResultCode ret =
      AddResponseMsg(std::move(request), [&](std::unique_ptr<BatchUserResponse> request) {
        batch_response = std::move(request);
        return;
      });

  if (ret == CollectorResultCode::STATE_CHANGED) {
    // LOG(ERROR) << "[X]STATE_CHANGED " << GetCurrentTime() - last_send_time_; 
    assert(batch_response);
    int next_primary = batch_response->next_primary();
    if (next_primary) {
      // One slot of the primary would get skipped.
      bool update = false;
      if (next_primary != primary_) {
        if (primary_ % 3 == 1 && primary_ < 3 * config_.GetForkTailNum()) {
          send_num_--;
          inflight_[primary_]--;
        }
        update = true;
      }
      primary_ = next_primary;
      if (update) {
        send_num_--;
        // first_slot_ = true;
      }
    }
    int primary_id = batch_response->primary_id();
    inflight_[primary_id]--;
    // LOG(ERROR) << "primary_id: " << primary_;
    SendResponseToClient(*batch_response);
  }
  return ret == CollectorResultCode::INVALID ? -2 : 0;
}


// int SlotHotStuff1PerformanceManager::GetPrimary(){
//   int view = counter_++ / slot_num_;
//   if (counter_ % slot_num_ == 0) {
//     counter_ += (client_num_ - 1) * slot_num_;
//   }
//   return view % replica_num_ + 1; 
// }

// int SlotHotStuff1PerformanceManager::GetPrimary() {
//   while (true) {
//     if (last_response_ != 0) {
//       break;
//     }
//   }
//   int value = last_response_;
//   while (true) {
//     if (inflight_[value] < inflight_limit_) {
//       inflight_[value]++;
//       break;
//     }
//     value = value % replica_num_ + 1;
//   }
//   return value;
// }

int SlotHotStuff1PerformanceManager::GetPrimary() {
  // LOG(ERROR) << "send to: " << primary_ << " send_num_:" << send_num_;
  // if (primary_ % 3 == 1 && primary_ < 3 * config_.GetRollBackNum() && first_slot_) {
  //   send_num_--;
  //   first_slot_ = false;
  // }
  //  LOG(ERROR) << "send to: " << primary_ << " send_num_:" << send_num_;
  return primary_;
}

void SlotHotStuff1PerformanceManager::SendResponseToClient(const BatchUserResponse& batch_response) {
  uint64_t create_time = batch_response.createtime();
  uint64_t primary_id = batch_response.primary_id();
  if (create_time > 0 && last_primary_id_ == primary_id) {
  // if (create_time > 0) {
    uint64_t run_time = GetCurrentTime() - create_time;
    // LOG(ERROR)<<"receive current:"<<GetCurrentTime()<<" create time:"<<create_time<<" run time:"<<run_time<<" local id:"<<batch_response.local_id();
    global_stats_->AddLatency(run_time);
  } else {
  }
  last_primary_id_ = primary_id;
  //send_num_-=10;
  send_num_--;
}

}  // namespace slot_hs1
}  // namespace resdb