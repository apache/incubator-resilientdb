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
