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
#include <mutex>
#include <queue>
#include <unordered_map>

#include "common/queue/lock_free_queue.h"
#include "config/resdb_config.h"
#include "execution/transaction_executor_impl.h"
#include "statistic/stats.h"

namespace resdb {

class GeoGlobalExecutor {
 public:
  GeoGlobalExecutor(std::unique_ptr<TransactionExecutorImpl> geo_executor_impl,
                    const ResDBConfig& config);
  virtual ~GeoGlobalExecutor();
  virtual void Execute(std::unique_ptr<Request> request);
  virtual int OrderGeoRequest(std::unique_ptr<Request> request);

  void Stop();

  std::unique_ptr<BatchClientResponse> GetResponseMsg();

 private:
  using ExecuteMap =
      std::map<std::pair<uint64_t, int>, std::unique_ptr<Request>>;

  bool IsStop();
  void OrderRound();
  std::unique_ptr<Request> GetNextMap();
  void AddData();

 protected:
  std::unique_ptr<TransactionExecutorImpl> geo_executor_impl_;
  Stats* global_stats_;
  std::thread execute_round_thread_, order_thread_;
  ExecuteMap execute_map_;
  uint64_t next_seq_ = 1;
  int next_region_ = 1;
  size_t region_size_;
  ResDBConfig config_;
  std::atomic<bool> is_stop_;
  std::mutex mutex_;
  std::condition_variable cv_;
  LockFreeQueue<Request> order_queue_;
  LockFreeQueue<BatchClientResponse> resp_queue_;
  int my_region_;
};
}  // namespace resdb
