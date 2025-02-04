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

#include "service/contract/executor/manager/local_view.h"

#include <glog/logging.h>
#include <gtest/gtest.h>

#include "service/contract/executor/manager/two_phase_controller.h"

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
