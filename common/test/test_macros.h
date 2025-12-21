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

#pragma once
#include <glog/logging.h>
#include <google/protobuf/util/json_util.h>
#include <google/protobuf/util/message_differencer.h>

#include "gmock/gmock.h"

namespace resdb {
namespace testing {

MATCHER_P(EqualsProto, replica, "") {
  return ::google::protobuf::util::MessageDifferencer::Equals(arg, replica);
}

template <typename T>
T ParseFromText(const std::string& json_str) {
  T message;
  google::protobuf::util::JsonParseOptions options;
  auto st =
      google::protobuf::util::JsonStringToMessage(json_str, &message, options);
  if (!st.ok()) {
    LOG(ERROR) << st.message();
  }
  return message;
}

}  // namespace testing
}  // namespace resdb
