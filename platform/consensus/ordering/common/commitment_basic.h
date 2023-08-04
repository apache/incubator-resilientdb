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

#include "platform/config/resdb_config.h"
#include "platform/consensus/ordering/common/message_manager_basic.h"
#include "platform/networkstrate/replica_communicator.h"
#include "platform/statistic/stats.h"

namespace resdb {

class CommitmentBasic {
 public:
  CommitmentBasic(const ResDBConfig& config,
                  MessageManagerBasic* transaction_manager,
                  ReplicaCommunicator* replica_communicator,
                  SignatureVerifier* verifier);
  virtual ~CommitmentBasic();

  void SetNodeId(const int32_t id);

 protected:
  virtual int PostProcessExecutedMsg();
  int32_t PrimaryId(int64_t view_num);

 protected:
  ResDBConfig config_;
  MessageManagerBasic* transaction_manager_;
  std::thread executed_thread_;
  std::atomic<bool> stop_;
  ReplicaCommunicator* replica_communicator_;
  SignatureVerifier* verifier_;

  int32_t id_;
  Stats* global_stats_ = nullptr;
};

}  // namespace resdb
