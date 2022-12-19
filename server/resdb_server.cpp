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

#include "server/resdb_server.h"

#include <glog/logging.h>
#include <signal.h>

#include <thread>

#include "common/network/tcp_socket.h"
#include "proto/broadcast.pb.h"

namespace resdb {

ResDBServer::ResDBServer(const ResDBConfig& config,
                         std::unique_ptr<ResDBService> service)
    : socket_(std::make_unique<TcpSocket>()),
      service_(std::move(service)),
      input_queue_("input"),
      resp_queue_("resp"),
      config_(config) {
  struct sigaction sa;
  sa.sa_handler = SIG_IGN;
  sa.sa_flags = 0;
  if (sigemptyset(&sa.sa_mask) == -1 || sigaction(SIGPIPE, &sa, 0) == -1) {
    perror("failed to ignore SIGPIPE; sigaction");
    exit(EXIT_FAILURE);
  }

  socket_->SetRecvTimeout(1000000);  // set 1s timeout.
  LOG(ERROR) << "listen ip:" << config.GetSelfInfo().ip()
             << " port:" << config.GetSelfInfo().port();
  assert(socket_->Listen(config.GetSelfInfo().ip(),
                         config.GetSelfInfo().port()) == 0);
  async_acceptor_ = std::make_unique<AsyncAcceptor>(
      config.GetSelfInfo().ip(), config_.GetSelfInfo().port() + 10000,
      config.GetInputWorkerNum(),
      std::bind(&ResDBServer::AcceptorHandler, this, std::placeholders::_1,
                std::placeholders::_2));
  async_acceptor_->StartAccept();
  global_stats_ = Stats::GetGlobalStats();
}

ResDBServer::~ResDBServer() {}

void ResDBServer::AcceptorHandler(const char* buffer, size_t data_len) {
  BroadcastData data;
  if (!data.ParseFromArray(buffer, data_len)) {
    LOG(ERROR) << "parse broad cast fail:" << data_len;
    return;
  }

  for (auto& sub_data : data.data()) {
    std::unique_ptr<DataInfo> sub_request_info = std::make_unique<DataInfo>();
    sub_request_info->data_len = sub_data.size();
    sub_request_info->buff = new char[sub_request_info->data_len];
    memcpy(sub_request_info->buff, sub_data.data(), sub_request_info->data_len);
    std::unique_ptr<QueueItem> item = std::make_unique<QueueItem>();
    item->socket = nullptr;
    item->data = std::move(sub_request_info);
    // LOG(ERROR) << "receve data from acceptor:" << data.is_resp()<<" data
    // len:"<<item->data->data_len;
    global_stats_->ServerCall();
    input_queue_.Push(std::move(item));
  }
}

void ResDBServer::InputProcess() {
  std::vector<std::thread> threads;

  int woker_num = config_.GetWorkerNum();
  LOG(ERROR) << "server:" << config_.GetSelfInfo().id() << " start running";
  for (int i = 0; i < woker_num; ++i) {
    threads.push_back(std::thread([&]() {
      while (IsRunning()) {
        std::unique_ptr<QueueItem> item = input_queue_.Pop(1000);
        if (item == nullptr) {
          continue;
        }
        global_stats_->ServerProcess();
        Process(std::move(item));
      }
    }));
  }

  for (auto& th : threads) {
    if (th.joinable()) {
      th.join();
    }
  }
}

void ResDBServer::Run() {
  service_->Start();

  auto input_th = std::thread(&ResDBServer::InputProcess, this);

  LockFreeQueue<Socket> socket_queue("server");
  std::vector<std::thread> threads;

  int woker_num = config_.GetInputWorkerNum();
  LOG(ERROR) << "server:" << config_.GetSelfInfo().id() << " start running";
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
        input_queue_.Push(std::move(item));
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
  input_th.join();
}

// Receive a message from network and pass it to service to process.
void ResDBServer::Process(std::unique_ptr<QueueItem> item) {
  auto client_socket =
      item->socket == nullptr ? nullptr : std::move(item->socket);
  auto request_info = std::move(item->data);
  if (client_socket != nullptr) {
    client_socket->SetSendTimeout(1000000);
  }
  std::unique_ptr<Context> context = std::make_unique<Context>();
  context->client = std::make_unique<ResDBClient>(std::move(client_socket),
                                                  /*connected=*/true);
  if (request_info) {
    service_->Process(std::move(context), std::move(request_info));
  }
  return;
}

bool ResDBServer::IsRunning() { return service_->IsRunning(); }

void ResDBServer::Stop() {
  socket_->Close();
  service_->Stop();
}

bool ResDBServer::ServiceIsReady() const { return service_->IsReady(); }

}  // namespace resdb
