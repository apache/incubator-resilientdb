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

#include "client/resdb_client.h"

#include <glog/logging.h>

#include "common/data_comm/data_comm.h"
#include "common/network/tcp_socket.h"

namespace resdb {

ResDBClient::ResDBClient(const std::string& ip, int port)
    : ip_(ip), port_(port) {
  socket_ = std::make_unique<TcpSocket>();
  socket_->SetSendTimeout(300);
  socket_->SetRecvTimeout(read_timeouts_);
}

void ResDBClient::SetRecvTimeout(int microseconds) {
  read_timeouts_ = microseconds;
  socket_->SetRecvTimeout(read_timeouts_);
}

void ResDBClient::SetAsyncSend(bool is_async_send) {
  is_async_send_ = is_async_send;
}

ResDBClient::ResDBClient(std::unique_ptr<Socket> socket, bool connected) {
  SetSocket(std::move(socket));
  connected_ = connected;
}

void ResDBClient::Close() { socket_->Close(); }

void ResDBClient::SetSignatureVerifier(SignatureVerifier* verifier) {
  verifier_ = verifier;
}

void ResDBClient::SetSocket(std::unique_ptr<Socket> socket) {
  socket_ = std::move(socket);
}

void ResDBClient::SetDestReplicaInfo(const ReplicaInfo& replica) {
  ip_ = replica.ip();
  port_ = replica.port();
}

void ResDBClient::IsLongConnection(bool long_connect_tion) {
  long_connect_tion_ = long_connect_tion;
}

int ResDBClient::Connect() {
  socket_->ReInit();
  socket_->SetAsync(is_async_send_);
  return socket_->Connect(ip_, port_);
}

int ResDBClient::SendDataInternal(const std::string& data) {
  return socket_->Send(data);
}

// Connect to the server if not connected and send data.
int ResDBClient::SendFromKeepAlive(const std::string& data) {
  for (int i = 0; i < max_retry_time_; ++i) {
    if (!long_connecting_) {
      if (Connect() == 0) {
        long_connecting_ = true;
      }
      if (!long_connecting_) {
        LOG(ERROR) << "connect fail:" << ip_ << " port:" << port_;
        continue;
      }
    }
    int ret = SendDataInternal(data);
    if (ret >= 0) {
      return ret;
    }
    long_connecting_ = false;
  }
  return -1;
}

int ResDBClient::Send(const std::string& data) {
  if (long_connect_tion_) {
    return SendFromKeepAlive(data);
  }
  for (int i = 0; i < max_retry_time_; ++i) {
    if (!connected_) {
      if (Connect()) {
        LOG(ERROR) << "connect fail:" << ip_ << " port:" << port_;
        continue;
      }
    }
    int ret = SendDataInternal(data);
    if (ret >= 0) {
      return ret;
    } else if (ret == -1) {
      LOG(ERROR) << "send data fail:" << ip_ << " port:" << port_;
    }
  }
  return -1;
}

// Receive data from the server.
int ResDBClient::Recv(std::string* data) {
  std::unique_ptr<DataInfo> resp = std::make_unique<DataInfo>();
  int ret = socket_->Recv(&resp->buff, &resp->data_len);
  if (ret > 0) {
    *data = std::string((char*)resp->buff, resp->data_len);
  }
  return ret;
}

// Sign the message if verifier has been provied and send the message to the
// server.
std::string ResDBClient::GetRawMessageString(
    const google::protobuf::Message& message, SignatureVerifier* verifier) {
  ResDBMessage sig_message;
  if (!message.SerializeToString(sig_message.mutable_data())) {
    return "";
  }

  if (verifier != nullptr) {
    auto signature_or = verifier->SignMessage(sig_message.data());
    if (!signature_or.ok()) {
      LOG(ERROR) << "Sign message fail";
      return "";
    }
    sig_message.mutable_signature()->Swap(&(*signature_or));
  } else {
    // LOG(ERROR) << " no verifier";
  }

  std::string message_str;
  if (!sig_message.SerializeToString(&message_str)) {
    return "";
  }
  return message_str;
}

int ResDBClient::SendRawMessageData(const std::string& message_str) {
  return Send(message_str);
}

int ResDBClient::RecvRawMessageData(std::string* message_str) {
  return Recv(message_str);
}

int ResDBClient::SendRawMessage(const google::protobuf::Message& message) {
  ResDBMessage sig_message;
  if (!message.SerializeToString(sig_message.mutable_data())) {
    return -1;
  }

  if (verifier_ != nullptr) {
    auto signature_or = verifier_->SignMessage(sig_message.data());
    if (!signature_or.ok()) {
      LOG(ERROR) << "Sign message fail";
      return -1;
    }
    sig_message.mutable_signature()->Swap(&(*signature_or));
  }
  std::string message_str;
  if (!sig_message.SerializeToString(&message_str)) {
    return -1;
  }
  return Send(message_str);
}

int ResDBClient::RecvRawMessageStr(std::string* message) {
  std::string recv_str;
  int ret = Recv(&recv_str);
  if (ret <= 0) {
    return ret;
  }
  ResDBMessage sig_message;
  if (!sig_message.ParseFromString(recv_str)) {
    LOG(ERROR) << "parse to sig fail";
    return -1;
  }
  *message = sig_message.data();
  return 0;
}

int ResDBClient::RecvRawMessage(google::protobuf::Message* message) {
  std::string resp_data;
  int ret = Recv(&resp_data);
  if (ret <= 0) {
    return -1;
  }
  ResDBMessage sig_message;
  if (!sig_message.ParseFromString(resp_data)) {
    LOG(ERROR) << "parse sig msg fail";
    return -1;
  }
  if (!message->ParseFromString(sig_message.data())) {
    LOG(ERROR) << "parse response msg fail";
    return -1;
  }
  return 0;
}

int ResDBClient::SendRequest(const google::protobuf::Message& message,
                             Request::Type type, bool need_response) {
  Request request;
  request.set_type(type);
  request.set_need_response(need_response);
  if (!message.SerializeToString(request.mutable_data())) {
    return -1;
  }
  return SendRawMessage(request);
}

}  // namespace resdb
