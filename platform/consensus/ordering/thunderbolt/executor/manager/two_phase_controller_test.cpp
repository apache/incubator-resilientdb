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

#include "service/contract/executor/manager/two_phase_controller.h"

#include <glog/logging.h>
#include <gtest/gtest.h>

namespace resdb {
namespace contract {
namespace {

using ::testing::Test;

uint256_t HexToInt(const std::string& v) { return eevm::to_uint256(v); }

void GetData(const std::string& addr, const DataStorage& storage,
             TwoPhaseController::ModifyMap& changes) {
  changes[HexToInt(addr)].push_back(Data(LOAD,
                                         storage.Load(HexToInt(addr)).first,
                                         storage.Load(HexToInt(addr)).second));
}

void GetData(const uint256_t& addr, const DataStorage& storage,
             TwoPhaseController::ModifyMap& changes) {
  changes[addr].push_back(
      Data(LOAD, storage.Load(addr).first, storage.Load(addr).second));
}

void SetData(const uint256_t& addr, int value,
             TwoPhaseController::ModifyMap& changes) {
  changes[addr].push_back(Data(STORE, value));
}

void SetData(const std::string& addr, int value,
             TwoPhaseController::ModifyMap& changes) {
  changes[HexToInt(addr)].push_back(Data(STORE, value));
}

TEST(TwoPhaseControllerTest, PushOneCommit) {
  DataStorage storage;
  TwoPhaseController controller(&storage);

  TwoPhaseController::ModifyMap changes;

  GetData("0x123", storage, changes);
  SetData("0x124", 1000, changes);

  controller.PushCommit(0, changes);

  controller.Commit(0);

  EXPECT_EQ(storage.Load(HexToInt("0x123")).first, 0);
  EXPECT_EQ(storage.Load(HexToInt("0x124")).first, 1000);
}

TEST(TwoPhaseControllerTest, SkipLOAD) {
  DataStorage storage;
  uint256_t address1 = HexToInt("0x123");
  uint256_t address2 = HexToInt("0x124");

  storage.Store(address1, 2000);
  EXPECT_EQ(storage.Load(address1).first, 2000);

  TwoPhaseController controller(&storage);

  TwoPhaseController::ModifyMap changes;

  GetData("0x123", storage, changes);
  SetData("0x124", 1000, changes);

  controller.PushCommit(0, changes);

  controller.Commit(0);

  EXPECT_EQ(storage.Load(address1).first, 2000);
  EXPECT_EQ(storage.Load(address2).first, 1000);
}

TEST(TwoPhaseControllerTest, Cover) {
  DataStorage storage;
  uint256_t address1 = HexToInt("0x123");

  storage.Store(address1, 2000);
  EXPECT_EQ(storage.Load(address1).first, 2000);

  TwoPhaseController controller(&storage);

  TwoPhaseController::ModifyMap changes;

  GetData(address1, storage, changes);
  SetData(address1, 1000, changes);

  controller.PushCommit(0, changes);

  EXPECT_TRUE(controller.Commit(0));

  EXPECT_EQ(storage.Load(address1).first, 1000);
}

TEST(TwoPhaseControllerTest, PushTwice) {
  DataStorage storage;
  uint256_t address1 = HexToInt("0x123");

  storage.Store(address1, 2000);
  EXPECT_EQ(storage.Load(address1).first, 2000);

  TwoPhaseController controller(&storage);

  TwoPhaseController::ModifyMap changes;

  GetData(address1, storage, changes);
  SetData(address1, 1000, changes);

  controller.PushCommit(0, changes);
  controller.PushCommit(0, changes);

  EXPECT_TRUE(controller.Commit(0));

  EXPECT_EQ(storage.Load(address1).first, 1000);
}

TEST(TwoPhaseControllerTest, CommitTwice) {
  DataStorage storage;
  uint256_t address1 = HexToInt("0x123");

  storage.Store(address1, 2000);
  EXPECT_EQ(storage.Load(address1).first, 2000);

  TwoPhaseController controller(&storage);

  TwoPhaseController::ModifyMap changes;
  GetData(address1, storage, changes);
  SetData(address1, 1000, changes);

  controller.PushCommit(0, changes);

  EXPECT_TRUE(controller.Commit(0));
  EXPECT_TRUE(controller.Commit(0));

  EXPECT_EQ(storage.Load(address1).first, 1000);
}

TEST(TwoPhaseControllerTest, ConflictFull) {
  DataStorage storage;
  uint256_t address1 = HexToInt("0x123");
  uint256_t address2 = HexToInt("0x124");

  storage.Store(address1, 2000);
  EXPECT_EQ(storage.Load(address1).first, 2000);

  TwoPhaseController controller(&storage);

  // commit 0:
  {
    TwoPhaseController::ModifyMap changes;
    GetData(address1, storage, changes);
    SetData(address2, 1000, changes);

    controller.PushCommit(0, changes);
  }

  // commit 1:
  {
    TwoPhaseController::ModifyMap changes;
    GetData(address1, storage, changes);
    SetData(address2, 3000, changes);

    controller.PushCommit(1, changes);
  }

  EXPECT_TRUE(controller.Commit(0));
  EXPECT_FALSE(controller.Commit(1));

  EXPECT_EQ(storage.Load(address1).first, 2000);
  EXPECT_EQ(storage.Load(address2).first, 1000);
}

TEST(TwoPhaseControllerTest, ConflictCommit1Again) {
  DataStorage storage;
  uint256_t address1 = HexToInt("0x123");
  uint256_t address2 = HexToInt("0x124");

  storage.Store(address1, 2000);
  EXPECT_EQ(storage.Load(address1).first, 2000);

  TwoPhaseController controller(&storage);

  // commit 1:
  {
    TwoPhaseController::ModifyMap changes;
    GetData(address1, storage, changes);
    SetData(address2, 3000, changes);

    controller.PushCommit(1, changes);
  }

  // commit 0:
  {
    TwoPhaseController::ModifyMap changes;
    GetData(address1, storage, changes);
    SetData(address2, 1000, changes);

    controller.PushCommit(0, changes);
  }

  EXPECT_TRUE(controller.Commit(0));
  EXPECT_FALSE(controller.Commit(1));

  EXPECT_EQ(storage.Load(address1).first, 2000);
  EXPECT_EQ(storage.Load(address2).first, 1000);
}

TEST(TwoPhaseControllerTest, ReadAfterWrite) {
  DataStorage storage;
  uint256_t address1 = HexToInt("0x123");
  uint256_t address2 = HexToInt("0x124");
  uint256_t address3 = HexToInt("0x125");

  storage.Store(address1, 2000);
  EXPECT_EQ(storage.Load(address1).first, 2000);

  TwoPhaseController controller(&storage);

  // commit 0:
  {
    TwoPhaseController::ModifyMap changes;
    GetData(address1, storage, changes);
    SetData(address2, 3000, changes);

    controller.PushCommit(0, changes);
  }

  // commit 1:
  {
    TwoPhaseController::ModifyMap changes;
    GetData(address2, storage, changes);
    SetData(address3, 1000, changes);

    controller.PushCommit(1, changes);
  }

  EXPECT_TRUE(controller.Commit(0));
  EXPECT_FALSE(controller.Commit(1));

  EXPECT_EQ(storage.Load(address1).first, 2000);
  EXPECT_EQ(storage.Load(address2).first, 3000);
  EXPECT_FALSE(storage.Exist(address3));
}

TEST(TwoPhaseControllerTest, WriteAfterWrite) {
  DataStorage storage;
  uint256_t address1 = HexToInt("0x123");
  uint256_t address2 = HexToInt("0x124");
  uint256_t address3 = HexToInt("0x125");

  storage.Store(address1, 2000);
  EXPECT_EQ(storage.Load(address1).first, 2000);

  TwoPhaseController controller(&storage);

  // commit 0:
  {
    TwoPhaseController::ModifyMap changes;
    GetData(address1, storage, changes);
    SetData(address2, 3000, changes);

    controller.PushCommit(0, changes);
  }

  // commit 1:
  {
    TwoPhaseController::ModifyMap changes;
    GetData(address2, storage, changes);
    SetData(address3, 1000, changes);

    controller.PushCommit(1, changes);
  }

  EXPECT_TRUE(controller.Commit(0));
  EXPECT_FALSE(controller.Commit(1));

  EXPECT_EQ(storage.Load(address1).first, 2000);
  EXPECT_EQ(storage.Load(address2).first, 3000);
  EXPECT_FALSE(storage.Exist(address3));
}

TEST(TwoPhaseControllerTest, ReadAfterRead) {
  DataStorage storage;
  uint256_t address1 = HexToInt("0x123");
  uint256_t address2 = HexToInt("0x124");
  uint256_t address3 = HexToInt("0x125");

  storage.Store(address1, 2000);
  EXPECT_EQ(storage.Load(address1).first, 2000);

  TwoPhaseController controller(&storage);

  // commit 0:
  {
    TwoPhaseController::ModifyMap changes;
    GetData(address1, storage, changes);
    SetData(address2, 3000, changes);

    controller.PushCommit(0, changes);
  }

  // commit 1:
  {
    TwoPhaseController::ModifyMap changes;
    GetData(address1, storage, changes);
    SetData(address3, 1000, changes);

    controller.PushCommit(1, changes);
  }

  EXPECT_TRUE(controller.Commit(0));
  EXPECT_TRUE(controller.Commit(1));

  EXPECT_EQ(storage.Load(address1).first, 2000);
  EXPECT_EQ(storage.Load(address2).first, 3000);
  EXPECT_EQ(storage.Load(address3).first, 1000);
}

TEST(TwoPhaseControllerTest, WriteAfterRead) {
  DataStorage storage;
  uint256_t address1 = HexToInt("0x123");
  uint256_t address2 = HexToInt("0x124");
  uint256_t address3 = HexToInt("0x125");

  storage.Store(address1, 2000);
  EXPECT_EQ(storage.Load(address1).first, 2000);

  TwoPhaseController controller(&storage);

  // commit 0:
  {
    TwoPhaseController::ModifyMap changes;
    GetData(address1, storage, changes);
    SetData(address2, 3000, changes);

    controller.PushCommit(0, changes);
  }

  // commit 1:
  {
    TwoPhaseController::ModifyMap changes;
    SetData(address1, 500, changes);
    SetData(address3, 1000, changes);

    controller.PushCommit(1, changes);
  }

  EXPECT_TRUE(controller.Commit(0));
  EXPECT_TRUE(controller.Commit(1));

  EXPECT_EQ(storage.Load(address1).first, 500);
  EXPECT_EQ(storage.Load(address2).first, 3000);
  EXPECT_EQ(storage.Load(address3).first, 1000);
}

TEST(TwoPhaseControllerTest, ReadAfterWriteAfterRead) {
  DataStorage storage;
  uint256_t address1 = HexToInt("0x123");
  uint256_t address2 = HexToInt("0x124");
  uint256_t address3 = HexToInt("0x125");
  uint256_t address4 = HexToInt("0x126");

  storage.Store(address1, 2000);
  EXPECT_EQ(storage.Load(address1).first, 2000);

  TwoPhaseController controller(&storage);

  // commit 0:
  {
    TwoPhaseController::ModifyMap changes;
    GetData(address1, storage, changes);
    SetData(address2, 3000, changes);

    controller.PushCommit(0, changes);
  }

  // commit 1:
  {
    TwoPhaseController::ModifyMap changes;
    SetData(address1, 500, changes);
    SetData(address3, 1000, changes);

    controller.PushCommit(1, changes);
  }

  // commit 2:
  {
    TwoPhaseController::ModifyMap changes;
    GetData(address1, storage, changes);
    SetData(address4, 2000, changes);

    controller.PushCommit(2, changes);
  }

  EXPECT_TRUE(controller.Commit(0));
  EXPECT_TRUE(controller.Commit(1));
  EXPECT_FALSE(controller.Commit(2));

  EXPECT_EQ(storage.Load(address1).first, 500);
  EXPECT_EQ(storage.Load(address2).first, 3000);
  EXPECT_EQ(storage.Load(address3).first, 1000);
  EXPECT_FALSE(storage.Exist(address4));
}

}  // namespace
}  // namespace contract
}  // namespace resdb
