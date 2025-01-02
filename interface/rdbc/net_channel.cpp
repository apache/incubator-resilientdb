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

#include "interface/rdbc/net_channel.h"

#include <glog/logging.h>

#include "platform/common/data_comm/data_comm.h"
#include "platform/common/network/tcp_socket.h"

namespace resdb {

NetChannel::NetChannel(const std::string& ip, int port) : ip_(ip), port_(port) {
  socket_ = std::make_unique<TcpSocket>();
  socket_->SetSendTimeout(300);
  socket_->SetRecvTimeout(read_timeouts_);
}

void NetChannel::SetRecvTimeout(int microseconds) {
  read_timeouts_ = microseconds;
  socket_->SetRecvTimeout(read_timeouts_);
}

void NetChannel::SetAsyncSend(bool is_async_send) {
  is_async_send_ = is_async_send;
}

NetChannel::NetChannel(std::unique_ptr<Socket> socket, bool connected) {
  SetSocket(std::move(socket));
  connected_ = connected;
}

void NetChannel::Close() { socket_->Close(); }

void NetChannel::SetSignatureVerifier(SignatureVerifier* verifier) {
  verifier_ = verifier;
}

void NetChannel::SetSocket(std::unique_ptr<Socket> socket) {
  socket_ = std::move(socket);
}

void NetChannel::SetDestReplicaInfo(const ReplicaInfo& replica) {
  ip_ = replica.ip();
  port_ = replica.port();
}

void NetChannel::IsLongConnection(bool long_connect_tion) {
  long_connect_tion_ = long_connect_tion;
}

int NetChannel::Connect() {
  socket_->ReInit();
  socket_->SetAsync(is_async_send_);
  return socket_->Connect(ip_, port_);
}

int NetChannel::SendDataInternal(const std::string& data) {
  return socket_->Send(data);
}

// Connect to the server if not connected and send data.
int NetChannel::SendFromKeepAlive(const std::string& data) {
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

int NetChannel::Send(const std::string& data) {
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
int NetChannel::Recv(std::string* data) {
  std::unique_ptr<DataInfo> resp = std::make_unique<DataInfo>();
  int ret = socket_->Recv(&resp->buff, &resp->data_len);
  if (ret > 0) {
    *data = std::string((char*)resp->buff, resp->data_len);
  }
  return ret;
}

// Sign the message if verifier has been provied and send the message to the
// server.
std::string NetChannel::GetRawMessageString(
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

int NetChannel::SendRawMessageData(const std::string& message_str) {
  return Send(message_str);
}

int NetChannel::RecvRawMessageData(std::string* message_str) {
  return Recv(message_str);
}

int NetChannel::SendRawMessage(const google::protobuf::Message& message) {
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

int NetChannel::RecvRawMessageStr(std::string* message) {
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

int NetChannel::RecvRawMessage(google::protobuf::Message* message) {
  std::string resp_data;
  int ret = Recv(&resp_data);
  if (ret < 0) {
    LOG(ERROR) << "recv fail:" << ret;
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

int NetChannel::SendRequest(const google::protobuf::Message& message,
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
