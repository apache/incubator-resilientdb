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
