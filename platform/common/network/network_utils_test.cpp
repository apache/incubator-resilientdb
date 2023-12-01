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
