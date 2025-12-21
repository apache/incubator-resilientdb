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

#include <gtest/gtest.h>

#include "common/test/test.pb.h"
#include "common/test/test_macros.h"

namespace resdb {
namespace testing {
namespace {

using ::resdb::testing::EqualsProto;

TEST(SignatureVerifyTest, ParseFromJson) {
  resdb::test::JsonMessage json_msg;

  json_msg.set_id(1);
  json_msg.set_data("data");
  json_msg.mutable_sub_msg()->set_id(2);
  json_msg.mutable_sub_msg()->set_data("sub_data");

  std::string json =
      "{ \
			    \"id\": 1, \
			    \"data\": \"data\", \
			    \"sub_msg\":{ \
				\"id\": 2, \
			        \"data\":\"sub_data\" \
			    } \
	}";

  EXPECT_THAT(ParseFromText<resdb::test::JsonMessage>(json),
              EqualsProto(json_msg));
}

}  // namespace
}  // namespace testing
}  // namespace resdb
