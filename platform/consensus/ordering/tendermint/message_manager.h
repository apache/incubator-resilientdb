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

#include "platform/common/queue/lock_free_queue.h"
#include "platform/config/resdb_config.h"
#include "platform/consensus/execution/system_info.h"
#include "platform/consensus/execution/transaction_executor.h"
#include "platform/consensus/ordering/tendermint/proto/tendermint.pb.h"

namespace resdb {
namespace tendermint {

class MessageManager {
 public:
  MessageManager(const ResDBConfig& config,
                 std::unique_ptr<TransactionManager> data_impl);

  int Commit(std::unique_ptr<TendermintRequest> tendermint_request);
  std::unique_ptr<BatchUserResponse> GetResponseMsg();

 private:
  ResDBConfig config_;
  SystemInfo system_info_;
  LockFreeQueue<BatchUserResponse> queue_;
  std::unique_ptr<TransactionExecutor> transaction_executor_;
  std::mutex mutex_;
};

}  // namespace tendermint
}  // namespace resdb
