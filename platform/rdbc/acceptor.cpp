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

  LOG(ERROR) << "listen ip:" << config.GetSelfInfo().ip()
             << " port:" << config.GetSelfInfo().port();
  assert(socket_->Listen(config.GetSelfInfo().ip(),
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
        input_queue_->Push(std::move(item));
      }
    }));
  }

  while (IsRunning()) {
    auto client_socket = socket_->Accept();
    if (client_socket == nullptr) {
      continue;
    }
    socket_queue.Push(std::move(client_socket));
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
