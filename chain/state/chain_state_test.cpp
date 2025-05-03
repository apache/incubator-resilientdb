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

#include "chain/state/chain_state.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "common/test/test_macros.h"

namespace resdb {
namespace {

using ::resdb::testing::EqualsProto;
using ::testing::Pointee;

TEST(ChainStateTest, GetEmptyValue) {
  ChainState db;
  EXPECT_EQ(db.Get(1), nullptr);
}

TEST(ChainStateTest, GetValue) {
  Request request;
  request.set_seq(1);
  request.set_data("test");

  ChainState db;
  db.Put(std::make_unique<Request>(request));
  EXPECT_THAT(db.Get(1), Pointee(EqualsProto(request)));
}

TEST(ChainStateTest, GetSecondValue) {
  Request request;
  request.set_seq(1);
  request.set_data("test");

  ChainState db;
  db.Put(std::make_unique<Request>(request));

  request.set_seq(1);
  request.set_data("test_1");
  db.Put(std::make_unique<Request>(request));

  EXPECT_THAT(db.Get(1), Pointee(EqualsProto(request)));
}

}  // namespace

}  // namespace resdb
