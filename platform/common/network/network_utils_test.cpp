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
