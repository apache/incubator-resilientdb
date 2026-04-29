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

#include "platform/rdbc/acceptor.h"

#include <glog/logging.h>
#include <signal.h>

#include <thread>

#include "platform/common/network/tcp_socket.h"

namespace resdb {

Acceptor::Acceptor(const ResDBConfig& config,
                   LockFreeQueue<QueueItem>* input_queue)
    : socket_(std::make_unique<TcpSocket>()),
      config_(config),
      input_queue_(input_queue) {
  socket_->SetRecvTimeout(1000000);  // set 1s timeout.

  LOG(ERROR) << "listen ip:" << "0.0.0.0"
             << " port:" << config.GetSelfInfo().port();
  assert(socket_->Listen("0.0.0.0",
                         config.GetSelfInfo().port()) == 0);
  is_stop_ = false;
  global_stats_ = Stats::GetGlobalStats();
}

Acceptor::~Acceptor() {}

void Acceptor::Run() {
  LOG(ERROR) << "server:" << config_.GetSelfInfo().id() << " start running";

  LockFreeQueue<Socket> socket_queue("server");
  std::vector<std::thread> threads;
  int woker_num = config_.GetInputWorkerNum();
  for (int i = 0; i < woker_num; ++i) {
    threads.push_back(std::thread([&]() {
      while (IsRunning()) {
        auto client_socket = socket_queue.Pop();
        if (client_socket == nullptr) {
          continue;
        }
        std::unique_ptr<DataInfo> request_info = std::make_unique<DataInfo>();
        int ret =
            client_socket->Recv(&request_info->buff, &request_info->data_len);
        if (ret <= 0) {
          continue;
        }
        std::unique_ptr<QueueItem> item = std::make_unique<QueueItem>();
        item->socket = std::move(client_socket);
        item->data = std::move(request_info);
        global_stats_->ServerCall();
        // Non-blocking push: if input_queue_ is full, drop the request
        // rather than blocking this Acceptor worker. Blocking here
        // freezes ALL client-facing connections when the InputProcess
        // workers can't drain fast enough (crash-fault transition).
        // The client will retry on timeout.
        if (!input_queue_->TryPush(std::move(item))) {
          // dropped — client retries
        }
      }
    }));
  }

  while (IsRunning()) {
    auto client_socket = socket_->Accept();
    if (client_socket == nullptr) {
      continue;
    }
    if (!socket_queue.TryPush(std::move(client_socket))) {
      // Connection dropped when overloaded — client retries
    }
  }

  for (auto& th : threads) {
    if (th.joinable()) {
      th.join();
    }
  }
}

void Acceptor::Stop() {
  is_stop_ = true;
  socket_->Close();
}

bool Acceptor::IsRunning() { return !is_stop_; }

}  // namespace resdb
