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

#include <boost/asio.hpp>

#include "interface/rdbc/net_channel.h"
#include "platform/common/queue/lock_free_queue.h"
#include "platform/proto/replica_info.pb.h"

namespace resdb {

class AsyncReplicaClient {
 public:
  AsyncReplicaClient(boost::asio::io_service* io_service, const std::string& ip,
                     int port, bool is_use_long_conn = false);
  virtual ~AsyncReplicaClient();

  virtual int SendMessage(const std::string& data);

 private:
  void ReConnect();
  void OnSendNewMessage();
  void OnSendMessage();
  void OnSend();

 private:
  LockFreeQueue<std::string> queue_;
  std::unique_ptr<NetChannel> client_;
  boost::asio::ip::tcp::socket socket_;
  boost::asio::ip::tcp::endpoint endpoint_;
  std::mutex mutex_;
  std::atomic<bool> in_process_;

  // ===== for async send =====
  std::unique_ptr<std::string> pending_data_;

  size_t data_size_ = 0;          // the size of the data needed to be sent.
  size_t sending_data_idx_ = 0;   // the current pos to be sent in data_ptr.
  size_t sending_data_size_ = 0;  // the size needed to be sent.
  const char* sending_data_ptr_ = nullptr;  // point to the data.

  int status_ = 0;  // sending status.
};

}  // namespace resdb
