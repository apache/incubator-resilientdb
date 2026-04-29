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

#include "platform/networkstrate/async_replica_client.h"

#include <glog/logging.h>
#include <boost/bind/bind.hpp>

#include "platform/common/queue/lock_free_queue.h"
#include "platform/proto/replica_info.pb.h"

namespace resdb {

AsyncReplicaClient::AsyncReplicaClient(boost::asio::io_service* io_service,
                                       const std::string& ip, int port,
                                       bool is_use_long_conn)
    : socket_(*io_service),
      endpoint_(boost::asio::ip::address::from_string(ip), port),
      io_service_(io_service),
      in_process_(false) {}

AsyncReplicaClient::~AsyncReplicaClient() {
  if (reconnect_timer_) {
    reconnect_timer_->cancel();
  }
}

int AsyncReplicaClient::SendMessage(const std::string& data) {
  // Circuit breaker: drop messages to known-dead connections instead of
  // queuing them (the queue would fill up and never drain).
  if (is_down_.load(std::memory_order_relaxed)) {
    return -1;
  }

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
    // Successful send resets the reconnect counter.
    reconnect_attempts_ = 0;
    OnSendNewMessage();
  }
}

void AsyncReplicaClient::OnSend() {
  socket_.async_write_some(
      boost::asio::buffer(sending_data_ptr_ + sending_data_idx_,
                          sending_data_size_ - sending_data_idx_),
      [this](const boost::system::error_code& error, size_t send_size) {
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
  // Close the broken socket so async_connect starts fresh.
  boost::system::error_code ec;
  socket_.close(ec);

  ++reconnect_attempts_;
  if (reconnect_attempts_ > max_reconnect_attempts_) {
    // Circuit breaker: mark this connection as DOWN.
    // Stop retrying immediately so the io_service thread is freed for
    // healthy connections. After a cooldown period, try once more.
    is_down_ = true;
    in_process_ = false;
    ScheduleReconnect();
    return;
  }

  socket_.async_connect(endpoint_,
      [this](const boost::system::error_code& error) {
        if (!error) {
          // Connection restored.
          reconnect_attempts_ = 0;
          is_down_ = false;
          status_ = 0;
          OnSendMessage();
        } else {
          // Failed again. Retry via async timer (NOT usleep which blocks
          // the io_service thread and starves all other connections).
          ScheduleReconnect();
        }
      });
}

void AsyncReplicaClient::ScheduleReconnect() {
  // Use an async timer so the io_service thread is NOT blocked.
  // The old code did usleep(10000) inside the async callback which blocked
  // the only worker thread and caused ALL connections to stall.
  if (!reconnect_timer_) {
    reconnect_timer_ = std::make_unique<boost::asio::steady_timer>(*io_service_);
  }

  int delay = is_down_ ? cooldown_ms_ : 10;  // 10ms between quick retries, 2s cooldown
  reconnect_timer_->expires_after(std::chrono::milliseconds(delay));
  reconnect_timer_->async_wait([this](const boost::system::error_code& error) {
    if (error) return;  // timer cancelled
    if (is_down_) {
      // Cooldown expired, try once more.
      is_down_ = false;
      reconnect_attempts_ = 0;
    }
    ReConnect();
  });
}

}  // namespace resdb
