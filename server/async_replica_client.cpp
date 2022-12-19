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

#include "server/async_replica_client.h"

#include <boost/bind/bind.hpp>

#include "client/resdb_client.h"
#include "common/queue/lock_free_queue.h"
#include "proto/replica_info.pb.h"

namespace resdb {

AsyncReplicaClient::AsyncReplicaClient(boost::asio::io_service* io_service,
                                       const std::string& ip, int port,
                                       bool is_use_long_conn)
    : socket_(*io_service),
      endpoint_(boost::asio::ip::address::from_string(ip), port),
      in_process_(false) {}

AsyncReplicaClient::~AsyncReplicaClient() {}

int AsyncReplicaClient::SendMessage(const std::string& data) {
  queue_.Push(std::make_unique<std::string>(data));
  if (!in_process_.load()) {
    bool old_value = false;
    if (in_process_.compare_exchange_strong(old_value, true,
                                            std::memory_order_acq_rel,
                                            std::memory_order_acq_rel)) {
      OnSendNewMessage();
    }
  }
  return 0;
}

void AsyncReplicaClient::OnSendNewMessage() {
  std::unique_ptr<std::string> data = queue_.Pop(0);
  if (data == nullptr || data->empty()) {
    in_process_ = false;
    return;
  }
  pending_data_ = std::move(data);
  status_ = 0;
  OnSendMessage();
}

void AsyncReplicaClient::OnSendMessage() {
  if (status_ == 0) {
    data_size_ = pending_data_->size();
    sending_data_size_ = sizeof(data_size_);
    sending_data_ptr_ = reinterpret_cast<char*>(&data_size_);
    sending_data_idx_ = 0;
    status_ = 1;
    OnSend();
  } else if (status_ == 1) {
    sending_data_size_ = data_size_;
    sending_data_ptr_ = pending_data_->c_str();
    sending_data_idx_ = 0;
    status_ = 2;
    OnSend();
  } else {
    status_ = 0;
    OnSendNewMessage();
  }
}

void AsyncReplicaClient::OnSend() {
  socket_.async_write_some(
      boost::asio::buffer(sending_data_ptr_ + sending_data_idx_,
                          sending_data_size_ - sending_data_idx_),
      [&](const boost::system::error_code& error, size_t send_size) {
        if (error) {
          ReConnect();
        } else {
          sending_data_idx_ += send_size;
          if (sending_data_idx_ >= sending_data_size_) {
            OnSendMessage();
          } else {
            OnSend();
          }
        }
      });
}

void AsyncReplicaClient::ReConnect() {
  socket_.async_connect(endpoint_, [&](const boost::system::error_code& error) {
    if (!error) {
      status_ = 0;
      OnSendMessage();
    } else {
      usleep(10000);
      ReConnect();
    }
  });
}

}  // namespace resdb
