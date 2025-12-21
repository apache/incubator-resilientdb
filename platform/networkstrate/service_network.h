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
#include "platform/common/network/socket.h"
#include "platform/common/queue/lock_free_queue.h"
#include "platform/config/resdb_config.h"
#include "platform/networkstrate/async_acceptor.h"
#include "platform/networkstrate/service_interface.h"
#include "platform/rdbc/acceptor.h"
#include "platform/statistic/stats.h"

namespace resdb {

// ServiceNetwork is a service running in BFT environment.
// It receives messages from other servers or clients and delivers them to
// ServiceInterface to process.
// service will be running in a multi-thread module.
class ServiceNetwork {
 public:
  // While running ServiceNetwork, it will lisenten to ip:port.
  ServiceNetwork(const ResDBConfig& config,
                 std::unique_ptr<ServiceInterface> service);
  virtual ~ServiceNetwork();

  // Run ServiceNetwork as background.
  void Run();
  void Stop();
  // Whether the service is ready to process the request.
  bool ServiceIsReady() const;

 private:
  void Process();
  void Process(std::unique_ptr<QueueItem> client_socket);
  bool IsRunning();
  void InputProcess();
  void AcceptorHandler(const char* buffer, size_t data_len);

 private:
  std::unique_ptr<Acceptor> acceptor_;
  std::unique_ptr<ServiceInterface> service_;
  bool is_running = false;
  LockFreeQueue<QueueItem> input_queue_, resp_queue_;
  std::unique_ptr<AsyncAcceptor> async_acceptor_;
  ResDBConfig config_;
  Stats* global_stats_;
};

}  // namespace resdb
