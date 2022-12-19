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

#include <string>

#include "common/network/socket.h"

namespace resdb {

// Socket for Tcp Server
class TcpSocket : public Socket {
 public:
  TcpSocket();
  TcpSocket(int socket_fd);
  ~TcpSocket();

  // For Server
  int Listen(const std::string& ip, int port) override;
  std::unique_ptr<Socket> Accept() override;
  virtual void ReInit();
  void Close() override;

  int Connect(const std::string& ip, int port) override;

  int Send(const std::string& data) override;
  int Recv(void** buf, size_t* len) override;

  int GetBindingPort() override;

  void SetRecvTimeout(int64_t microseconds) override;
  void SetSendTimeout(int64_t microseconds) override;
  int SetAsync(bool is_open = true) override;

 private:
  int InitSocket();

 private:
  int socket_fd_;
  int binding_port_ = 0;
};
}  // namespace resdb
