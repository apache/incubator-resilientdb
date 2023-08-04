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

#include <future>

#include "platform/consensus/ordering/common/performance_generator.h"

namespace resdb {
namespace hotstuff {

class PerformanceManager : public PerformancGeneratorBasic {
 public:
  PerformanceManager(const ResDBConfig& config,
                     ReplicaCommunicator* replica_communicator,
                     SystemInfo* system_info, SignatureVerifier* verifier)
      : PerformancGeneratorBasic(config, replica_communicator, system_info,
                                 verifier) {}

  void ConverToRequest(const BatchUserRequest& batch_request,
                       Request* new_request) override;
  void PostSend();
};

}  // namespace hotstuff
}  // namespace resdb
