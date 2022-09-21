#pragma once
#include <google/protobuf/util/message_differencer.h>

#include "gmock/gmock.h"

namespace resdb {
namespace testing {

MATCHER_P(EqualsProto, replica, "") {
  return ::google::protobuf::util::MessageDifferencer::Equals(arg, replica);
}

}  // namespace testing
}  // namespace resdb
