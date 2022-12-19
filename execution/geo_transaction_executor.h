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
#include "config/resdb_config.h"
#include "execution/system_info.h"
#include "execution/transaction_executor_impl.h"
#include "server/resdb_replica_client.h"

namespace resdb {

class GeoTransactionExecutor : public TransactionExecutorImpl {
 public:
  GeoTransactionExecutor(
      const ResDBConfig& config, std::unique_ptr<SystemInfo> system_info,
      std::unique_ptr<ResDBReplicaClient> replica_client,
      std::unique_ptr<TransactionExecutorImpl> geo_executor_impl);

  virtual ~GeoTransactionExecutor();
  virtual std::unique_ptr<BatchClientResponse> ExecuteBatch(
      const BatchClientRequest& request) override;
  bool IsStop();

 private:
  void SendGeoMessages();
  void SendBatchGeoMessage(
      const std::vector<std::unique_ptr<Request>>& requests);

 protected:
  ResDBConfig config_;
  std::unique_ptr<SystemInfo> system_info_;
  std::unique_ptr<ResDBReplicaClient> replica_client_;
  std::unique_ptr<TransactionExecutorImpl> geo_executor_impl_ = nullptr;
  std::thread geo_thread_;
  size_t batch_size_ = 50;
  LockFreeQueue<Request> queue_;
  std::atomic<bool> is_stop_;

  std::vector<std::unique_ptr<BatchClientRequest>> messages_;
};

}  // namespace resdb
