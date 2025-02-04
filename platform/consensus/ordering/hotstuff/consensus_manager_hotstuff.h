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
#include "platform/consensus/ordering/hotstuff/commitment.h"
#include "platform/consensus/ordering/hotstuff/message_manager.h"
#include "platform/consensus/ordering/hotstuff/performance_manager.h"
#include "platform/consensus/ordering/hotstuff/response_manager.h"
#include "platform/networkstrate/consensus_manager.h"

namespace resdb {
namespace hotstuff {

class ConsensusManagerHotStuff : public ConsensusManager {
 public:
  ConsensusManagerHotStuff(const ResDBConfig& config,
                           std::unique_ptr<TransactionManager> executor);
  virtual ~ConsensusManagerHotStuff() = default;

  int ConsensusCommit(std::unique_ptr<Context> context,
                      std::unique_ptr<Request> request) override;

  std::vector<ReplicaInfo> GetReplicas() override;

  void Start();
  void SetupPerformanceDataFunc(std::function<std::string()> func);

 protected:
  SystemInfo system_info_;
  std::unique_ptr<MessageManager> message_manager_;
  std::unique_ptr<Commitment> commitment_;
  std::unique_ptr<ResponseManager> response_manager_;
  std::unique_ptr<PerformanceManager> performance_manager_;
};

}  // namespace hotstuff
}  // namespace resdb
