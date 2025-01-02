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

#include "network/network_utils.h"

namespace resdb {

std::string GetDNSName(const std::string& ip, int port, NetworkType type) {
  char dns_name[1024];
  switch (type) {
    case TPORT_TYPE:
      snprintf(dns_name, sizeof(dns_name), "ipc://node_%d.ipc", port);
      break;
    case ENVIRONMENT_EC2:
      snprintf(dns_name, sizeof(dns_name), "tcp://0.0.0.0:%d", port);
      break;
    default:
      snprintf(dns_name, sizeof(dns_name), "tcp://%s:%d", ip.data(), port);
      break;
  }
  return dns_name;
}

std::string GetTcpUrl(const std::string& ip, int port) {
  char name[1024];
  if (port > 0) {
    snprintf(name, sizeof(name), "tcp://%s:%d", ip.data(), port);
  } else {
    snprintf(name, sizeof(name), "tcp://%s", ip.data());
  }
  return name;
}

}  // namespace resdb
