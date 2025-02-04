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
