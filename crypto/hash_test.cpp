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

#include "crypto/hash.h"

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
