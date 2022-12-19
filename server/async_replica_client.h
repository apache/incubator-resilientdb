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

#include <boost/asio.hpp>

#include "client/resdb_client.h"
#include "common/queue/lock_free_queue.h"
#include "proto/replica_info.pb.h"

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
  std::unique_ptr<ResDBClient> client_;
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
