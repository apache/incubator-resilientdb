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
#include "platform/consensus/ordering/thunderbolt/executor/paral_sm/local_view.h"

#include <glog/logging.h>
#include <gtest/gtest.h>

#include "platform/consensus/ordering/thunderbolt/executor/paral_sm/two_phase_controller.h"

namespace resdb {
namespace contract {
namespace {

using ::testing::Test;

uint256_t HexToInt(const std::string& v) { return eevm::to_uint256(v); }

TEST(LocalViewTest, ViewChange) {
  DataStorage storage;
  uint256_t address1 = HexToInt("0x123");

  storage.Store(address1, 2000);
  EXPECT_EQ(storage.Load(address1).first, 2000);

  TwoPhaseController controller(&storage);

  LocalView view(&controller, 0);

  EXPECT_EQ(view.load(address1), 2000);
  view.store(address1, 3000);
  EXPECT_EQ(view.load(address1), 3000);

  // storage still contains 2000
  EXPECT_EQ(storage.Load(address1).first, 2000);
}

TEST(LocalViewTest, CommitChange) {
  DataStorage storage;
  uint256_t address1 = HexToInt("0x123");

  storage.Store(address1, 2000);
  EXPECT_EQ(storage.Load(address1).first, 2000);

  TwoPhaseController controller(&storage);

  LocalView view(&controller, 0);

  EXPECT_EQ(view.load(address1), 2000);
  view.store(address1, 3000);
  EXPECT_EQ(view.load(address1), 3000);

  // storage still contains 2000
  EXPECT_EQ(storage.Load(address1).first, 2000);

  view.Flesh(0);
  EXPECT_TRUE(controller.Commit(0));
  // Save to real storage.
  EXPECT_EQ(storage.Load(address1).first, 3000);
}

TEST(LocalViewTest, CommitConflict) {
  DataStorage storage;
  uint256_t address1 = HexToInt("0x123");

  storage.Store(address1, 2000);
  EXPECT_EQ(storage.Load(address1).first, 2000);

  TwoPhaseController controller(&storage);

  LocalView view1(&controller, 0);
  LocalView view2(&controller, 1);

  EXPECT_EQ(view1.load(address1), 2000);
  view1.store(address1, 3000);
  EXPECT_EQ(view1.load(address1), 3000);

  // storage still contains 2000
  EXPECT_EQ(storage.Load(address1).first, 2000);

  EXPECT_EQ(view2.load(address1), 2000);
  view2.store(address1, 4000);
  EXPECT_EQ(view2.load(address1), 4000);

  // storage still contains 2000
  EXPECT_EQ(storage.Load(address1).first, 2000);

  view1.Flesh(0);
  view2.Flesh(1);

  EXPECT_FALSE(controller.Commit(1));
  EXPECT_TRUE(controller.Commit(0));

  // Save to real storage.
  EXPECT_EQ(storage.Load(address1).first, 3000);
}

}  // namespace
}  // namespace contract
}  // namespace resdb
