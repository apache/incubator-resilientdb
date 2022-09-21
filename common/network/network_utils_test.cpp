#include "network/network_utils.h"

#include <gtest/gtest.h>

#include <numeric>
#include <thread>
#include <vector>

namespace resdb {

TEST(NetworkUtilsTest, GetDNSName) {
  EXPECT_EQ(GetDNSName("127.0.0.1", 1234, NetworkType::TPORT_TYPE),
            "ipc://node_1234.ipc");
  EXPECT_EQ(GetDNSName("127.0.0.1", 1234, NetworkType::ENVIRONMENT_EC2),
            "tcp://0.0.0.0:1234");
  EXPECT_EQ(GetDNSName("127.0.0.1", 1234, NetworkType::TCP),
            "tcp://127.0.0.1:1234");
}

TEST(NetworkUtilsTest, GetTcpUrl) {
  EXPECT_EQ(GetTcpUrl("127.0.0.1", 1234), "tcp://127.0.0.1:1234");
  EXPECT_EQ(GetTcpUrl("127.0.0.1:1234"), "tcp://127.0.0.1:1234");
}
}  // namespace resdb
