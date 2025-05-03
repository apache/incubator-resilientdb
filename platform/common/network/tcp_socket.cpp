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

#include "platform/common/network/tcp_socket.h"

#include <arpa/inet.h>
#include <errno.h>
#include <glog/logging.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <thread>

namespace resdb {

namespace {

int RecvInternal(int fd, void* buf, size_t len) {
  size_t pos = 0;
  while (len > 0) {
    int ret = recv(fd, (char*)buf + pos, len, 0);
    if (ret <= 0) {
      // LOG(ERROR) << "recv data fail, fd =" << fd << " error " <<
      // strerror(errno)
      //          << " ret:" << ret;
      return ret;
    }
    pos += ret;
    len -= ret;
  }
  return pos;
}

int SendInternal(int fd, const void* buf, size_t len) {
  if (fd < 0) {
    return -2;
  }
  size_t pos = 0;
  while (pos < len) {
    int ret = send(fd, (char*)buf + pos, len - pos, 0);
    if (ret < 0) {
      LOG(ERROR) << "send data fail, fd =" << fd << " error "
                 << strerror(errno);
      return -1;
    }
    pos += ret;
  }
  return pos;
}

}  // namespace

TcpSocket::TcpSocket() : socket_fd_(-1) { InitSocket(); }

TcpSocket::TcpSocket(int socket_fd) : socket_fd_(socket_fd) {}

TcpSocket::~TcpSocket() { Close(); }

void TcpSocket::ReInit() {
  Close();
  InitSocket();
}

void TcpSocket::Close() {
  if (socket_fd_ >= 0) {
    close(socket_fd_);
    socket_fd_ = -1;
  }
}

int TcpSocket::InitSocket() {
  if ((socket_fd_ = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    LOG(ERROR) << "create TcpSocket error: " << strerror(errno)
               << " (errno: " << errno << ")";
    return -1;
  }
  return 0;
}

int TcpSocket::Listen(const std::string& ip, int port) {
  int on = 1;
  if (setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) != 0) {
    LOG(ERROR) << "set TcpSocket opt fail, error" << strerror(errno);
    return -1;
  }

  struct sockaddr_in servaddr;
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = inet_addr(ip.data());
  servaddr.sin_port = htons(port);

  if (bind(socket_fd_, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
    LOG(ERROR) << "bind TcpSocket error: " << strerror(errno)
               << "(errno: " << errno << ")";
    return -1;
  }

  if (listen(socket_fd_, 10) == -1) {
    LOG(ERROR) << "listen TcpSocket error: " << strerror(errno)
               << "(errno: " << errno << ")";
    return -1;
  }
  if (port == 0) {
    struct sockaddr_in localaddr;
    socklen_t len = sizeof(localaddr);
    int ret = getsockname(socket_fd_, (struct sockaddr*)&localaddr, &len);
    if (ret != 0) {
      LOG(ERROR) << "get binding port fail:" << strerror(errno);
    } else {
      binding_port_ = ntohs(localaddr.sin_port);
    }
  } else {
    binding_port_ = port;
  }
  return 0;
}

int TcpSocket::GetBindingPort() { return binding_port_; }

std::unique_ptr<Socket> TcpSocket::Accept() {
  int conn_fd = 0;
  if ((conn_fd = accept(socket_fd_, (struct sockaddr*)NULL, NULL)) == -1) {
    // LOG(ERROR) << "accept:" << socket_fd_
    //         << " TcpSocket error: " << strerror(errno) << "(errno: " <<
    //         errno
    //        << ")" << std::this_thread::get_id();
    return nullptr;
  }
  return std::make_unique<TcpSocket>(conn_fd);
}

int TcpSocket::Connect(const std::string& ip, int port) {
  if (socket_fd_ < 0) {
    LOG(ERROR) << "socket fd invalid:" << socket_fd_;
    return -1;
  }
  struct sockaddr_in servaddr;
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = inet_addr(ip.data());
  servaddr.sin_port = htons(port);

  if (connect(socket_fd_, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
    LOG(ERROR) << "fd:" << socket_fd_ << " connect ip:" << ip
               << " port:" << port << " error: " << strerror(errno)
               << "(errno: " << errno << ")";
    return -1;
  }
  // LOG(ERROR) << "connect to:" << ip << " " << port;

  return 0;
}

int TcpSocket::SetAsync(bool is_open) {
  unsigned long ul = is_open;
  return ioctl(socket_fd_, FIONBIO, (unsigned long*)&ul);
}

int TcpSocket::Send(const std::string& data) {
  size_t data_size = data.size();
  size_t send_ret = SendInternal(socket_fd_, &data_size, sizeof(data_size));
  if (send_ret != sizeof(data_size)) {
    if (static_cast<int>(send_ret) == -1) {
      LOG(ERROR) << "send data error: " << strerror(errno)
                 << "(errno: " << errno << ")";
      return -1;
    }
    return -2;
  }

  send_ret = SendInternal(socket_fd_, data.c_str(), data_size);
  if (send_ret != data_size) {
    if (static_cast<int>(send_ret) == -1) {
      LOG(ERROR) << "send data not done: " << strerror(errno)
                 << "(errno: " << errno << ")";
      return -1;
    } else {
      return -2;
    }
  }
  return 0;
}

int TcpSocket::Recv(void** buf, size_t* len) {
  int ret = RecvInternal(socket_fd_, len, sizeof(size_t));
  if (ret <= 0) {
    return ret;
  }
  *buf = malloc(*len);
  ret = RecvInternal(socket_fd_, *buf, *len);
  if (ret <= 0) {
    return ret;
  }
  return ret;
}

void TcpSocket::SetRecvTimeout(int64_t microseconds) {
  if (socket_fd_ < 0) {
    return;
  }
  struct timeval timeout = {microseconds / 1000000,
                            static_cast<int>(microseconds % 1000000)};
  if (setsockopt(socket_fd_, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout,
                 sizeof(struct timeval)) != 0) {
    LOG(ERROR) << "set recv timeout:" << microseconds
               << " failed:" << strerror(errno);
  }
}

void TcpSocket::SetSendTimeout(int64_t microseconds) {
  struct timeval timeout = {microseconds / 1000000,
                            static_cast<int>(microseconds % 1000000)};
  if (setsockopt(socket_fd_, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout,
                 sizeof(struct timeval)) != 0) {
    LOG(ERROR) << "set second timeout failed:" << strerror(errno);
  }
}

}  // namespace resdb
