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

#include "common/data_comm/data_comm.h"
#include "common/network/socket.h"
#include "common/queue/lock_free_queue.h"
#include "config/resdb_config.h"
#include "server/async_acceptor.h"
#include "server/resdb_service.h"
#include "statistic/stats.h"

namespace resdb {

struct QueueItem {
  std::unique_ptr<Socket> socket;
  std::unique_ptr<DataInfo> data;
};
// ResDBServer is a service running in BFT environment.
// It receives messages from other servers or clients and delivers them to
// ResDBService to process.
// service will be running in a multi-thread module.
class ResDBServer {
 public:
  // While running ResDBServer, it will lisenten to ip:port.
  ResDBServer(const ResDBConfig& config, std::unique_ptr<ResDBService> service);
  virtual ~ResDBServer();

  // Run ResDBServer as background.
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
  std::unique_ptr<Socket> socket_;
  std::unique_ptr<ResDBService> service_;
  bool is_running = false;
  LockFreeQueue<QueueItem> input_queue_, resp_queue_;
  std::unique_ptr<AsyncAcceptor> async_acceptor_;
  ResDBConfig config_;
  Stats* global_stats_;
};

}  // namespace resdb
