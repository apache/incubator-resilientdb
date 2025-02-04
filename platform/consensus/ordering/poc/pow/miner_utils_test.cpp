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
