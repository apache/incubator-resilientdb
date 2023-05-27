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
        LOG(ERROR) << "push item";
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
