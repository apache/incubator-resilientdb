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

#include <stdint.h>

#include <map>
#include <memory>
#include <queue>
#include <set>

#include "platform/config/resdb_config.h"
#include "platform/consensus/ordering/poe/nomac/commitment.h"
#include "platform/consensus/ordering/poe/nomac/message_manager.h"
#include "platform/networkstrate/replica_communicator.h"

namespace resdb {
namespace poe {

class ViewChange {
 public:
  ViewChange(const ResDBConfig& config, MessageManager* message_manager,
             Commitment* commitment, SystemInfo* system_info,
             ReplicaCommunicator* replica_communicator,
             SignatureVerifier* verifier);
  ~ViewChange();

  int ProcessViewChange(std::unique_ptr<ViewChangeMsg> vc_msg);
  int ProcessNewView(std::unique_ptr<NewViewChangeMsg> new_vc);

  bool VerifyNodeSignagure(const POERequest& request);
  bool VerifyTS(const NewViewChangeMsg& request);

 private:
  void Notify();
  void WaitOrStop();
  void Monitor();
  QC GetQC(uint64_t view);
  void TriggerViewChange(uint64_t new_view);
  void SendNewView(uint64_t new_view);

 private:
  ResDBConfig config_;
  std::atomic<bool> is_stop_;
  std::condition_variable view_change_cv_;
  std::mutex view_change_mutex_, mutex_;
  std::thread monitor_thread_;
  std::atomic<bool> has_new_req_, is_inited_;
  MessageManager* message_manager_;
  Commitment* commitment_;
  SystemInfo* system_info_;
  ReplicaCommunicator* replica_communicator_;
  SignatureVerifier* verifier_;
  std::map<uint64_t, std::set<int>> new_view_msgs_;
  std::map<uint64_t, std::vector<std::unique_ptr<ViewChangeMsg>>>
      new_view_requests_;
  std::atomic<int> vc_count_;
};

}  // namespace poe
}  // namespace resdb
