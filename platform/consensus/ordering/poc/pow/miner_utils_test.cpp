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

#include "platform/consensus/ordering/poc/pow/miner_utils.h"

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <boost/format.hpp>

#include "common/crypto/signature_verifier.h"
#include "common/test/test_macros.h"

namespace resdb {
namespace {

using ::google::protobuf::util::MessageDifferencer;
using ::testing::ElementsAre;
using ::testing::Pair;
using ::testing::Test;

TEST(MinerUtilsTest, DigestToHashValue) {
  std::string str_value;
  str_value += std::to_string(100);
  str_value += std::to_string(1);

  std::string ret = SignatureVerifier::CalculateHash(
      SignatureVerifier::CalculateHash(str_value));

  HashValue hash_value = DigestToHash(ret);

  std::string hash_digest = GetHashDigest(hash_value);

  EXPECT_EQ(hash_digest, ret);
}

TEST(MinerUtilsTest, CmpLeadingZerosFromRawDigest) {
  std::string str_value;
  str_value += std::to_string(100);
  str_value += std::to_string(1);

  for (int i = 1; i <= 64; ++i) {
    std::string ret = SignatureVerifier::CalculateHash(
        SignatureVerifier::CalculateHash(str_value));

    int j = 0;
    for (j = 0; j < (i + 1) / 2; ++j) {
      ret[j] = 0x00;
    }

    if (i % 2 != 0) {
      ret[j - 1] = 0x0f;
    } else {
      ret[j] = 0xff;
    }

    EXPECT_TRUE(IsValidDigest(ret, i * 4));
    if (i + 1 <= 64) {
      EXPECT_FALSE(IsValidDigest(ret, (i + 1) * 4));
    }
  }
}

TEST(MinerUtilsTest, DigestString) {
  std::string str_value;
  str_value += std::to_string(100);
  str_value += std::to_string(1);

  std::string ret = SignatureVerifier::CalculateHash(
      SignatureVerifier::CalculateHash(str_value));
  ret[0] = 0x03;
  std::string hex_string = GetDigestHexString(ret);
  EXPECT_EQ(hex_string,
            "039411d480d5867a111f06aaf9e5cba7fd5ebd730a54962d5752f74dc49b0898");
}

}  // namespace
}  // namespace resdb
