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

#include "common/crypto/hash.h"

#include <glog/logging.h>
#include <gtest/gtest.h>

#include "common/test/test_macros.h"

namespace resdb {
namespace utils {
namespace {

using ::resdb::testing::EqualsProto;

TEST(SignatureVerifyTest, CalculateSHA256) {
  std::string expected_str =
      "\x9F\x86\xD0\x81\x88L}e\x9A/"
      "\xEA\xA0\xC5Z\xD0\x15\xA3\xBFO\x1B+\v\x82,\xD1]l\x15\xB0\xF0\n\b";
  EXPECT_EQ(CalculateSHA256Hash("test"), expected_str);
}

TEST(SignatureVerifyTest, CalculateRIPEMD160) {
  std::string expected_str =
      "^R\xFE\xE4~k\a\x5"
      "e\xF7"
      "CrF\x8C\xDCi\x9D\xE8\x91\a";
  EXPECT_EQ(CalculateRIPEMD160Hash("test"), expected_str);
}

}  // namespace
}  // namespace utils
}  // namespace resdb
