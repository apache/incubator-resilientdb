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
#include <memory>

namespace resdb {

class AsyncAcceptor {
 public:
  typedef std::function<void(const char* buffer, size_t len)> CallBack;

  AsyncAcceptor(const std::string& ip, int thread_num, int port,
                CallBack call_back_func);
  virtual ~AsyncAcceptor();

  void StartAccept();

 private:
  class Session {
   public:
    Session(boost::asio::io_service* io_service, CallBack call_back_func);
    ~Session();

    boost::asio::ip::tcp::socket* GetSocket();

    void StartRead();
    void Close();

   private:
    void ReadDone();
    void OnRead();

   private:
    boost::asio::io_service* io_service_ = nullptr;
    boost::asio::ip::tcp::socket client_socket_;
    size_t data_size_ = 0;
    size_t need_size_ = 0;
    size_t current_idx_ = 0;
    char* recv_buffer_ = nullptr;
    bool status_ = 0;
    CallBack call_back_func_;
  };

 private:
  void OnAccept(boost::shared_ptr<Session> client_socket,
                const boost::system::error_code ec);

 private:
  boost::asio::io_service io_service_;
  boost::asio::ip::tcp::endpoint endpoint_;
  boost::asio::ip::tcp::acceptor acceptor_;
  std::unique_ptr<boost::asio::io_service::work> worker_;
  CallBack call_back_func_;
  std::vector<std::thread> worker_thread_;
  std::vector<boost::shared_ptr<Session>> sessions_;
};

}  // namespace resdb
