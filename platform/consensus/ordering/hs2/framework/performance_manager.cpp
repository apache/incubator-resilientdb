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

#include "platform/consensus/ordering/hs2/framework/performance_manager.h"

#include <glog/logging.h>

#include "common/utils/utils.h"

namespace resdb {
namespace hs2 {

using comm::CollectorResultCode;

HotStuff2PerformanceManager::HotStuff2PerformanceManager(
    const ResDBConfig& config, ReplicaCommunicator* replica_communicator,
    SignatureVerifier* verifier)
    : PerformanceManager(config, replica_communicator, verifier){
  client_num_ = 1;
  primary_ = id_ % replica_num_;
}

int HotStuff2PerformanceManager::GetPrimary(){
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


}  // namespace hs2
}  // namespace resdb