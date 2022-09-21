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
