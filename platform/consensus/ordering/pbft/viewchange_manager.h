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

#include "common/crypto/signature_verifier.h"
#include "platform/config/resdb_config.h"
#include "platform/consensus/execution/system_info.h"
#include "platform/consensus/ordering/pbft/checkpoint_manager.h"
#include "platform/consensus/ordering/pbft/message_manager.h"
#include "platform/networkstrate/replica_communicator.h"
#include "platform/proto/viewchange_message.pb.h"

namespace resdb {

class ViewChangeManager {
 public:
  ViewChangeManager(const ResDBConfig& config,
                    CheckPointManager* checkpoint_manager,
                    MessageManager* message_manager, SystemInfo* system_info,
                    ReplicaCommunicator* replica_communicator,
                    SignatureVerifier* verifier);
  virtual ~ViewChangeManager();

  int ProcessViewChange(std::unique_ptr<Context> context,
                        std::unique_ptr<Request> request);
  int ProcessNewView(std::unique_ptr<Context> context,
                     std::unique_ptr<Request> request);
  bool IsInViewChange();
  // If the monitor is not running, start to monitor.
  void MayStart();

  enum ViewChangeStatus {
    NONE = 0,
    READY_VIEW_CHANGE = 1,
    READY_NEW_VIEW = 2,
  };

 private:
  void SendViewChangeMsg();
  void SendNewViewMsg(uint64_t view_number);
  bool IsValidViewChangeMsg(const ViewChangeMessage& view_change_message);
  uint32_t AddRequest(const ViewChangeMessage& viewchange_message,
                      uint32_t sender);
  bool IsNextPrimary(uint64_t view_number);
  void SetCurrentViewAndNewPrimary(uint64_t view_number);
  std::vector<std::unique_ptr<Request>> GetPrepareMsg(
      const NewViewMessage& new_view_message, bool need_sign = true);

  bool ChangeStatue(ViewChangeStatus status);

 protected:
  ResDBConfig config_;
  CheckPointManager* checkpoint_manager_;
  MessageManager* message_manager_;
  SystemInfo* system_info_;
  ReplicaCommunicator* replica_communicator_;
  SignatureVerifier* verifier_;
  std::thread monitor_thread_;
  std::map<uint64_t, std::map<uint32_t, ViewChangeMessage>> viewchange_request_;
  std::mutex mutex_, status_mutex_;
  bool new_view_is_sent_ = false;
  ViewChangeStatus status_;
  std::atomic<bool> started_;
  uint32_t view_change_counter_;
};

}  // namespace resdb
