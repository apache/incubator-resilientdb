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
#include <memory>

#include "platform/common/data_comm/data_comm.h"
#include "platform/common/data_comm/network_comm.h"
#include "platform/common/network/socket.h"
#include "platform/common/queue/lock_free_queue.h"
#include "platform/config/resdb_config.h"
#include "platform/statistic/stats.h"

namespace resdb {

// Acceptor is a service running in BFT environment.
// It receives messages from other servers or clients and delivers them to
// ServiceInterface to process.
// service will be running in a multi-thread module.
class Acceptor {
 public:
  // While running Acceptor, it will lisenten to ip:port.
  Acceptor(const ResDBConfig& config, LockFreeQueue<QueueItem>* input_queue);
  virtual ~Acceptor();

  // Run Acceptor as background.
  void Run();
  void Stop();

 private:
  bool IsRunning();

 private:
  std::unique_ptr<Socket> socket_;
  ResDBConfig config_;
  LockFreeQueue<QueueItem>* input_queue_;
  Stats* global_stats_;
  std::atomic<bool> is_stop_;
};

}  // namespace resdb
