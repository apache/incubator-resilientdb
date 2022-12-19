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

#include "common/logging/logging.h"

#include <gtest/gtest.h>

#include "common/test/test_macros.h"
#include "proto/logging.pb.h"

namespace resdb {

using ::resdb::testing::EqualsProto;

TEST(Logging, log) {
  Logging logging("./test_log.log");
  logging.Clear();
  {
    Logging logging("./test_log.log");

    LoggingInfo info;
    info.set_seq(123);
    info.set_request("request");
    info.set_signature("signature");

    logging.Log(info);
  }
  {
    LoggingInfo expected_info;
    expected_info.set_seq(123);
    expected_info.set_request("request");
    expected_info.set_signature("signature");

    Logging logging("./test_log.log");
    logging.ResetHead();

    LoggingInfo info;
    logging.Read(&info);
    EXPECT_THAT(info, EqualsProto(expected_info));

    int ret = logging.Read(&info);
    EXPECT_NE(ret, 0);
    logging.ResetEnd();

    info.set_seq(234);
    logging.Log(info);
  }
  {
    LoggingInfo expected_info;
    expected_info.set_seq(123);
    expected_info.set_request("request");
    expected_info.set_signature("signature");

    Logging logging("./test_log.log");
    logging.ResetHead();

    LoggingInfo info;
    logging.Read(&info);
    EXPECT_THAT(info, EqualsProto(expected_info));

    expected_info.set_seq(234);
    logging.Read(&info);
    EXPECT_THAT(info, EqualsProto(expected_info));

    int ret = logging.Read(&info);
    EXPECT_NE(ret, 0);
    logging.ResetEnd();
  }
}

}  // namespace resdb
