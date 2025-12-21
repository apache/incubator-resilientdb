/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
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
