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
#include "platform/consensus/ordering/thunderbolt/executor/manager/sequential_cc_controller.h"

#include <glog/logging.h>
#include <gtest/gtest.h>

#include <future>

namespace resdb {
namespace contract {
namespace {

using ::testing::Test;

uint256_t HexToInt(const std::string& v) { return eevm::to_uint256(v); }

void GetData(const std::string& addr, const DataStorage& storage,
             ConcurrencyController::ModifyMap& changes) {
  changes[HexToInt(addr)].push_back(Data(LOAD,
                                         storage.Load(HexToInt(addr)).first,
                                         storage.Load(HexToInt(addr)).second));
}

void SetData(const std::string& addr, int value,
             ConcurrencyController::ModifyMap& changes) {
  changes[HexToInt(addr)].push_back(Data(STORE, value));
}

TEST(ContractDagManagerTest, PushOneCommit) {
  std::promise<bool> done;
  std::future<bool> done_future = done.get_future();

  SequentialCCController::CallBack call_back;
  call_back.committed_callback = [&](int64_t) {
    LOG(ERROR) << "committed";
    done.set_value(true);
  };

  int64_t commit_id = 1;

  DataStorage storage;
  SequentialCCController controller(&storage);
  controller.SetCallback(call_back);

  SequentialCCController::ModifyMap changes;
  GetData("0x123", storage, changes);
  SetData("0x124", 1000, changes);

  controller.PushCommit(commit_id, changes);

  done_future.get();
  EXPECT_EQ(storage.Load(HexToInt("0x123")).first, 0);
  EXPECT_EQ(storage.Load(HexToInt("0x124")).first, 1000);
}

TEST(ContractDagManagerTest, PushTwoOOOCommit) {
  std::promise<bool> done;
  std::future<bool> done_future = done.get_future();

  int committed_num = 0;
  SequentialCCController::CallBack call_back;
  call_back.committed_callback = [&](int64_t) {
    LOG(ERROR) << "committed";
    committed_num++;
    if (committed_num == 2) {
      done.set_value(true);
    }
  };

  DataStorage storage;
  SequentialCCController controller(&storage);
  controller.SetCallback(call_back);

  {
    int64_t commit_id = 2;

    SequentialCCController::ModifyMap changes;

    SetData("0x123", 3000, changes);
    SetData("0x124", 4000, changes);

    controller.PushCommit(commit_id, changes);
  }

  {
    int64_t commit_id = 1;

    SequentialCCController::ModifyMap changes;
    SetData("0x123", 1000, changes);
    SetData("0x124", 2000, changes);

    controller.PushCommit(commit_id, changes);
  }

  done_future.get();
  EXPECT_EQ(storage.Load(HexToInt("0x123")).first, 3000);
  EXPECT_EQ(storage.Load(HexToInt("0x124")).first, 4000);
}

TEST(ContractDagManagerTest, PushTwoOOOCommitRAWBlock) {
  DataStorage storage;
  SequentialCCController controller(&storage);

  std::promise<bool> done;
  std::future<bool> done_future = done.get_future();

  int committed1_done = 0, redo_num = 0;
  SequentialCCController::CallBack call_back;
  call_back.committed_callback = [&](int64_t commit_id) {
    if (commit_id == 1) {
      committed1_done = 1;
    }
  };

  call_back.redo_callback = [&](int64_t commit_id) {
    EXPECT_EQ(commit_id, 2);
    redo_num = 1;
    if (committed1_done == 1 && redo_num == 1) {
      done.set_value(true);
    }
    LOG(ERROR) << "redo:" << commit_id;
  };

  controller.SetCallback(call_back);
  {
    int64_t commit_id = 2;

    SequentialCCController::ModifyMap changes;
    GetData("0x123", storage, changes);
    SetData("0x124", 4000, changes);

    controller.PushCommit(commit_id, changes);
  }

  {
    int64_t commit_id = 1;

    SequentialCCController::ModifyMap changes;
    SetData("0x123", 1000, changes);
    SetData("0x124", 2000, changes);

    controller.PushCommit(commit_id, changes);
  }

  done_future.get();
  EXPECT_EQ(storage.Load(HexToInt("0x123")).first, 1000);
  EXPECT_EQ(storage.Load(HexToInt("0x124")).first, 2000);
}

TEST(ContractDagManagerTest, PushTwoOOOCommitRAWRedo) {
  DataStorage storage;
  SequentialCCController controller(&storage);

  std::promise<bool> done;
  std::future<bool> done_future = done.get_future();

  int committed1_done = 0, committed2_done = 0, redo_num = 0;
  SequentialCCController::CallBack call_back;
  call_back.committed_callback = [&](int64_t commit_id) {
    if (commit_id == 1) {
      committed1_done = 1;
    }
    if (commit_id == 2) {
      committed2_done = 1;
    }
    if (committed1_done == 1 && committed2_done == 1 && redo_num == 1) {
      done.set_value(true);
    }
  };

  call_back.redo_callback = [&](int64_t commit_id) {
    EXPECT_EQ(commit_id, 2);
    redo_num = 1;
    LOG(ERROR) << "redo:" << commit_id;
    SequentialCCController::ModifyMap changes;

    GetData("0x123", storage, changes);
    SetData("0x124", 4000, changes);

    controller.PushCommit(commit_id, changes);
  };

  controller.SetCallback(call_back);
  {
    int64_t commit_id = 2;

    SequentialCCController::ModifyMap changes;
    GetData("0x123", storage, changes);
    SetData("0x124", 4000, changes);

    controller.PushCommit(commit_id, changes);
  }

  {
    int64_t commit_id = 1;

    SequentialCCController::ModifyMap changes;
    SetData("0x123", 1000, changes);
    SetData("0x124", 2000, changes);

    controller.PushCommit(commit_id, changes);
  }

  done_future.get();
  EXPECT_EQ(storage.Load(HexToInt("0x123")).first, 1000);
  EXPECT_EQ(storage.Load(HexToInt("0x124")).first, 4000);
}

TEST(ContractDagManagerTest, PushTwoOOOCommitRAWRedo2) {
  DataStorage storage;
  SequentialCCController controller(&storage);

  storage.Store(HexToInt("0x120"), 0000);
  storage.Store(HexToInt("0x121"), 1000);
  storage.Store(HexToInt("0x122"), 2000);
  storage.Store(HexToInt("0x123"), 3000);
  storage.Store(HexToInt("0x124"), 4000);

  std::promise<bool> done;
  std::future<bool> done_future = done.get_future();

  int committed1_done = 0, committed2_done = 0, redo_num = 0;
  SequentialCCController::CallBack call_back;
  call_back.committed_callback = [&](int64_t commit_id) {
    if (commit_id == 1) {
      committed1_done = 1;
    }
    if (commit_id == 2) {
      committed2_done = 1;
    }
    if (committed1_done == 1 && committed2_done == 1 && redo_num == 1) {
      done.set_value(true);
    }
  };

  call_back.redo_callback = [&](int64_t commit_id) {
    EXPECT_EQ(commit_id, 2);
    redo_num = 1;
    LOG(ERROR) << "redo:" << commit_id;

    SequentialCCController::ModifyMap changes;
    GetData("0x122", storage, changes);
    GetData("0x123", storage, changes);
    SetData("0x124", 5000, changes);

    controller.PushCommit(commit_id, changes);
  };

  controller.SetCallback(call_back);
  {
    int64_t commit_id = 2;

    SequentialCCController::ModifyMap changes;
    GetData("0x122", storage, changes);
    GetData("0x123", storage, changes);
    SetData("0x124", 5000, changes);

    controller.PushCommit(commit_id, changes);
  }

  {
    int64_t commit_id = 1;

    SequentialCCController::ModifyMap changes;
    SetData("0x123", 1000, changes);
    SetData("0x124", 6000, changes);

    controller.PushCommit(commit_id, changes);
  }

  done_future.get();
  EXPECT_EQ(storage.Load(HexToInt("0x121")).first, 1000);
  EXPECT_EQ(storage.Load(HexToInt("0x122")).first, 2000);
  EXPECT_EQ(storage.Load(HexToInt("0x123")).first, 1000);
  EXPECT_EQ(storage.Load(HexToInt("0x124")).first, 5000);
}

TEST(ContractDagManagerTest, ThreeOOOCommitRAWRedo) {
  DataStorage storage;
  SequentialCCController controller(&storage);

  storage.Store(HexToInt("0x120"), 0000);
  storage.Store(HexToInt("0x121"), 1000);
  storage.Store(HexToInt("0x122"), 2000);
  storage.Store(HexToInt("0x123"), 3000);
  storage.Store(HexToInt("0x124"), 4000);

  std::promise<bool> done;
  std::future<bool> done_future = done.get_future();
  std::set<int64_t> done_list;
  int redo_num = 0;
  SequentialCCController::CallBack call_back;
  call_back.committed_callback = [&](int64_t commit_id) {
    done_list.insert(commit_id);
    if (done_list.size() == 3 && redo_num == 1) {
      done.set_value(true);
    }
  };

  call_back.redo_callback = [&](int64_t commit_id) {
    EXPECT_EQ(commit_id, 3);
    redo_num = 1;
    LOG(ERROR) << "redo:" << commit_id;

    SequentialCCController::ModifyMap changes;

    GetData("0x121", storage, changes);
    GetData("0x122", storage, changes);
    GetData("0x123", storage, changes);
    SetData("0x125", 8000, changes);

    controller.PushCommit(commit_id, changes);
  };

  controller.SetCallback(call_back);

  {
    int64_t commit_id = 3;

    SequentialCCController::ModifyMap changes;
    GetData("0x121", storage, changes);
    GetData("0x122", storage, changes);
    GetData("0x123", storage, changes);
    SetData("0x125", 8000, changes);

    controller.PushCommit(commit_id, changes);
  }
  {
    int64_t commit_id = 2;

    SequentialCCController::ModifyMap changes;
    SetData("0x123", 5000, changes);
    SetData("0x124", 5000, changes);

    controller.PushCommit(commit_id, changes);
  }

  {
    int64_t commit_id = 1;

    SequentialCCController::ModifyMap changes;
    SetData("0x122", 5000, changes);
    SetData("0x124", 6000, changes);

    controller.PushCommit(commit_id, changes);
  }

  done_future.get();
  EXPECT_EQ(storage.Load(HexToInt("0x121")).first, 1000);
  EXPECT_EQ(storage.Load(HexToInt("0x122")).first, 5000);
  EXPECT_EQ(storage.Load(HexToInt("0x123")).first, 5000);
  EXPECT_EQ(storage.Load(HexToInt("0x124")).first, 5000);
  EXPECT_EQ(storage.Load(HexToInt("0x125")).first, 8000);
}

TEST(ContractDagManagerTest, ThreeOOOCommitRAW2Redo) {
  DataStorage storage;
  SequentialCCController controller(&storage);

  storage.Store(HexToInt("0x120"), 0000);
  storage.Store(HexToInt("0x121"), 1000);
  storage.Store(HexToInt("0x122"), 2000);
  storage.Store(HexToInt("0x123"), 3000);
  storage.Store(HexToInt("0x124"), 4000);

  std::promise<bool> done;
  std::future<bool> done_future = done.get_future();
  std::set<int64_t> done_list;
  int redo_num = 0;
  SequentialCCController::CallBack call_back;
  call_back.committed_callback = [&](int64_t commit_id) {
    LOG(ERROR) << "commit:" << commit_id;
    done_list.insert(commit_id);
    if (done_list.size() == 3 && redo_num == 2) {
      done.set_value(true);
    }
  };

  call_back.redo_callback = [&](int64_t commit_id) {
    redo_num++;
    LOG(ERROR) << "redo:" << commit_id;

    SequentialCCController::ModifyMap changes;

    if (commit_id == 3) {
      GetData("0x120", storage, changes);
      GetData("0x122", storage, changes);
      SetData("0x125", 8000, changes);
    } else if (commit_id == 2) {
      GetData("0x121", storage, changes);
      SetData("0x123", 5000, changes);
      SetData("0x120", 5000, changes);
    }
    controller.PushCommit(commit_id, changes);
  };

  controller.SetCallback(call_back);
  {
    int64_t commit_id = 3;

    SequentialCCController::ModifyMap changes;
    GetData("0x120", storage, changes);
    GetData("0x122", storage, changes);
    SetData("0x125", 8000, changes);

    controller.PushCommit(commit_id, changes);
  }
  {
    int64_t commit_id = 2;

    SequentialCCController::ModifyMap changes;
    GetData("0x121", storage, changes);
    SetData("0x123", 5000, changes);
    SetData("0x120", 5000, changes);

    controller.PushCommit(commit_id, changes);
  }

  {
    int64_t commit_id = 1;

    SequentialCCController::ModifyMap changes;
    SetData("0x121", 5000, changes);
    SetData("0x122", 5000, changes);
    SetData("0x124", 6000, changes);

    controller.PushCommit(commit_id, changes);
  }

  done_future.get();
  EXPECT_EQ(storage.Load(HexToInt("0x120")).first, 5000);
  EXPECT_EQ(storage.Load(HexToInt("0x121")).first, 5000);
  EXPECT_EQ(storage.Load(HexToInt("0x122")).first, 5000);
  EXPECT_EQ(storage.Load(HexToInt("0x123")).first, 5000);
  EXPECT_EQ(storage.Load(HexToInt("0x124")).first, 6000);
  EXPECT_EQ(storage.Load(HexToInt("0x125")).first, 8000);
}

TEST(ContractDagManagerTest, ThreeRAW2Redo3list) {
  DataStorage storage;
  SequentialCCController controller(&storage);

  storage.Store(HexToInt("0x120"), 0000);
  storage.Store(HexToInt("0x121"), 1000);
  storage.Store(HexToInt("0x122"), 2000);
  storage.Store(HexToInt("0x123"), 3000);
  storage.Store(HexToInt("0x124"), 4000);
  storage.Store(HexToInt("0x125"), 5000);
  storage.Store(HexToInt("0x126"), 6000);

  std::promise<bool> done;
  std::future<bool> done_future = done.get_future();
  std::set<int64_t> done_list;
  int redo_num = 0;
  SequentialCCController::CallBack call_back;
  call_back.committed_callback = [&](int64_t commit_id) {
    done_list.insert(commit_id);
    if (done_list.size() == 3 && redo_num == 2) {
      done.set_value(true);
    }
  };

  call_back.redo_callback = [&](int64_t commit_id) {
    redo_num++;
    LOG(ERROR) << "redo:" << commit_id;

    SequentialCCController::ModifyMap changes;

    if (commit_id == 3) {
      GetData("0x120", storage, changes);
      GetData("0x122", storage, changes);
      SetData("0x125", 8000, changes);
    } else if (commit_id == 2) {
      GetData("0x121", storage, changes);
      SetData("0x123", 5000, changes);
      SetData("0x120", 5000, changes);
    }
    controller.PushCommit(commit_id, changes);
  };
  controller.SetCallback(call_back);

  {
    int64_t commit_id = 3;

    SequentialCCController::ModifyMap changes;
    GetData("0x126", storage, changes);
    GetData("0x121", storage, changes);
    SetData("0x125", 8000, changes);

    controller.PushCommit(commit_id, changes);
  }
  {
    int64_t commit_id = 2;

    SequentialCCController::ModifyMap changes;
    GetData("0x121", storage, changes);
    SetData("0x123", 5000, changes);
    SetData("0x120", 5000, changes);

    controller.PushCommit(commit_id, changes);
  }

  {
    int64_t commit_id = 1;

    SequentialCCController::ModifyMap changes;
    SetData("0x121", 5000, changes);
    SetData("0x122", 5000, changes);
    SetData("0x124", 6000, changes);

    controller.PushCommit(commit_id, changes);
  }

  done_future.get();
  EXPECT_EQ(storage.Load(HexToInt("0x120")).first, 5000);
  EXPECT_EQ(storage.Load(HexToInt("0x121")).first, 5000);
  EXPECT_EQ(storage.Load(HexToInt("0x122")).first, 5000);
  EXPECT_EQ(storage.Load(HexToInt("0x123")).first, 5000);
  EXPECT_EQ(storage.Load(HexToInt("0x124")).first, 6000);
  EXPECT_EQ(storage.Load(HexToInt("0x125")).first, 8000);
  EXPECT_EQ(storage.Load(HexToInt("0x126")).first, 6000);
}

TEST(ContractDagManagerTest, TwoOOORAWRedoAfterNew) {
  DataStorage storage;
  SequentialCCController controller(&storage);

  storage.Store(HexToInt("0x120"), 0000);
  storage.Store(HexToInt("0x121"), 1000);
  storage.Store(HexToInt("0x122"), 2000);
  storage.Store(HexToInt("0x123"), 3000);
  storage.Store(HexToInt("0x124"), 4000);

  std::promise<bool> done;
  std::future<bool> done_future = done.get_future();

  int redo_num = 0;
  std::set<int64_t> done_list;
  SequentialCCController::CallBack call_back;
  call_back.committed_callback = [&](int64_t commit_id) {
    done_list.insert(commit_id);
    if (done_list.size() == 3 && redo_num == 1) {
      done.set_value(true);
    }
  };

  call_back.redo_callback = [&](int64_t commit_id) {
    EXPECT_EQ(commit_id, 2);
    redo_num = 1;
    LOG(ERROR) << "redo:" << commit_id;

    if (commit_id == 2) {
      {
        int64_t new_commit_id = 3;

        SequentialCCController::ModifyMap changes;
        GetData("0x120", storage, changes);
        SetData("0x121", 4000, changes);

        controller.PushCommit(new_commit_id, changes);
      }

      SequentialCCController::ModifyMap changes;
      GetData("0x122", storage, changes);
      SetData("0x124", 2000, changes);
      controller.PushCommit(commit_id, changes);
    }
  };

  controller.SetCallback(call_back);
  {
    int64_t commit_id = 2;

    SequentialCCController::ModifyMap changes;
    GetData("0x122", storage, changes);
    SetData("0x124", 2000, changes);

    controller.PushCommit(commit_id, changes);
  }

  {
    int64_t commit_id = 1;

    SequentialCCController::ModifyMap changes;
    SetData("0x122", 1000, changes);
    SetData("0x124", 5000, changes);

    controller.PushCommit(commit_id, changes);
  }

  done_future.get();
  EXPECT_EQ(storage.Load(HexToInt("0x120")).first, 0000);
  EXPECT_EQ(storage.Load(HexToInt("0x121")).first, 4000);
  EXPECT_EQ(storage.Load(HexToInt("0x122")).first, 1000);
  EXPECT_EQ(storage.Load(HexToInt("0x123")).first, 3000);
  EXPECT_EQ(storage.Load(HexToInt("0x124")).first, 2000);
}

TEST(ContractDagManagerTest, TwoOOORAWRedoAfterNewConflict) {
  DataStorage storage;
  SequentialCCController controller(&storage);

  storage.Store(HexToInt("0x120"), 0000);
  storage.Store(HexToInt("0x121"), 1000);
  storage.Store(HexToInt("0x122"), 2000);
  storage.Store(HexToInt("0x123"), 3000);
  storage.Store(HexToInt("0x124"), 4000);

  std::promise<bool> done;
  std::future<bool> done_future = done.get_future();

  int redo_num = 0;
  std::set<int64_t> done_list;
  SequentialCCController::CallBack call_back;
  call_back.committed_callback = [&](int64_t commit_id) {
    done_list.insert(commit_id);
    if (done_list.size() == 3 && redo_num == 2) {
      done.set_value(true);
    }
  };

  call_back.redo_callback = [&](int64_t commit_id) {
    redo_num++;
    LOG(ERROR) << "redo:" << commit_id;

    if (commit_id == 2) {
      {
        int64_t new_commit_id = 3;

        SequentialCCController::ModifyMap changes;
        GetData("0x124", storage, changes);
        SetData("0x121", 4000, changes);

        controller.PushCommit(new_commit_id, changes);
      }

      SequentialCCController::ModifyMap changes;
      GetData("0x122", storage, changes);
      SetData("0x124", 2000, changes);
      controller.PushCommit(commit_id, changes);
    } else if (commit_id == 3) {
      SequentialCCController::ModifyMap changes;
      GetData("0x124", storage, changes);
      SetData("0x121", 4000, changes);

      controller.PushCommit(commit_id, changes);
    }
  };

  controller.SetCallback(call_back);
  {
    int64_t commit_id = 2;

    SequentialCCController::ModifyMap changes;
    GetData("0x122", storage, changes);
    SetData("0x124", 2000, changes);

    controller.PushCommit(commit_id, changes);
  }

  {
    int64_t commit_id = 1;

    SequentialCCController::ModifyMap changes;
    SetData("0x122", 1000, changes);
    SetData("0x124", 5000, changes);

    controller.PushCommit(commit_id, changes);
  }

  done_future.get();
  EXPECT_EQ(storage.Load(HexToInt("0x120")).first, 0000);
  EXPECT_EQ(storage.Load(HexToInt("0x121")).first, 4000);
  EXPECT_EQ(storage.Load(HexToInt("0x122")).first, 1000);
  EXPECT_EQ(storage.Load(HexToInt("0x123")).first, 3000);
  EXPECT_EQ(storage.Load(HexToInt("0x124")).first, 2000);
}

TEST(ContractDagManagerTest, PushTwoOOOCommitRAWNoRedo) {
  DataStorage storage;
  SequentialCCController controller(&storage);

  storage.Store(HexToInt("0x122"), 2000);

  std::promise<bool> done;
  std::future<bool> done_future = done.get_future();

  int committed1_done = 0, committed2_done = 0, redo_num = 0;
  SequentialCCController::CallBack call_back;
  call_back.committed_callback = [&](int64_t commit_id) {
    if (commit_id == 1) {
      committed1_done = 1;
    }
    if (commit_id == 2) {
      committed2_done = 1;
    }
    if (committed1_done == 1 && committed2_done == 1 && redo_num == 0) {
      done.set_value(true);
    }
  };

  call_back.redo_callback = [&](int64_t commit_id) {
    EXPECT_EQ(commit_id, 2);
    redo_num = 1;
    LOG(ERROR) << "redo:" << commit_id;
  };

  controller.SetCallback(call_back);
  {
    int64_t commit_id = 2;

    SequentialCCController::ModifyMap changes;
    GetData("0x122", storage, changes);
    SetData("0x124", 4000, changes);

    controller.PushCommit(commit_id, changes);
  }

  {
    int64_t commit_id = 1;

    SequentialCCController::ModifyMap changes;
    SetData("0x123", 1000, changes);
    SetData("0x124", 4000, changes);

    controller.PushCommit(commit_id, changes);
  }

  done_future.get();
  EXPECT_EQ(storage.Load(HexToInt("0x122")).first, 2000);
  EXPECT_EQ(storage.Load(HexToInt("0x123")).first, 1000);
  EXPECT_EQ(storage.Load(HexToInt("0x124")).first, 4000);
}

TEST(ContractDagManagerTest, PushTwoOOOCommitWAWNoRedo) {
  DataStorage storage;
  SequentialCCController controller(&storage);

  storage.Store(HexToInt("0x120"), 0000);
  storage.Store(HexToInt("0x121"), 1000);
  storage.Store(HexToInt("0x122"), 2000);
  storage.Store(HexToInt("0x123"), 3000);
  storage.Store(HexToInt("0x124"), 4000);

  std::promise<bool> done;
  std::future<bool> done_future = done.get_future();

  int committed1_done = 0, committed2_done = 0, redo_num = 0;
  SequentialCCController::CallBack call_back;
  call_back.committed_callback = [&](int64_t commit_id) {
    if (commit_id == 1) {
      committed1_done = 1;
    }
    if (commit_id == 2) {
      committed2_done = 1;
    }
    if (committed1_done == 1 && committed2_done == 1 && redo_num == 0) {
      done.set_value(true);
    }
  };

  call_back.redo_callback = [&](int64_t commit_id) {
    EXPECT_EQ(commit_id, 2);
    redo_num = 1;
    LOG(ERROR) << "redo:" << commit_id;
  };

  controller.SetCallback(call_back);
  {
    int64_t commit_id = 2;

    SequentialCCController::ModifyMap changes;
    SetData("0x122", 3000, changes);
    SetData("0x124", 6000, changes);

    controller.PushCommit(commit_id, changes);
  }

  {
    int64_t commit_id = 1;

    SequentialCCController::ModifyMap changes;
    SetData("0x123", 2000, changes);
    SetData("0x124", 1000, changes);

    controller.PushCommit(commit_id, changes);
  }

  done_future.get();
  EXPECT_EQ(storage.Load(HexToInt("0x120")).first, 0000);
  EXPECT_EQ(storage.Load(HexToInt("0x121")).first, 1000);
  EXPECT_EQ(storage.Load(HexToInt("0x122")).first, 3000);
  EXPECT_EQ(storage.Load(HexToInt("0x123")).first, 2000);
  EXPECT_EQ(storage.Load(HexToInt("0x124")).first, 6000);
}

TEST(ContractDagManagerTest, PushTwoOOOCommitWARNoRedo) {
  DataStorage storage;
  SequentialCCController controller(&storage);

  storage.Store(HexToInt("0x120"), 0000);
  storage.Store(HexToInt("0x121"), 1000);
  storage.Store(HexToInt("0x122"), 2000);
  storage.Store(HexToInt("0x123"), 3000);
  storage.Store(HexToInt("0x124"), 4000);

  std::promise<bool> done;
  std::future<bool> done_future = done.get_future();

  int committed1_done = 0, committed2_done = 0, redo_num = 0;
  SequentialCCController::CallBack call_back;
  call_back.committed_callback = [&](int64_t commit_id) {
    if (commit_id == 1) {
      committed1_done = 1;
    }
    if (commit_id == 2) {
      committed2_done = 1;
    }
    if (committed1_done == 1 && committed2_done == 1 && redo_num == 0) {
      done.set_value(true);
    }
  };

  call_back.redo_callback = [&](int64_t commit_id) {
    EXPECT_EQ(commit_id, 2);
    redo_num = 1;
    LOG(ERROR) << "redo:" << commit_id;
  };

  controller.SetCallback(call_back);
  {
    int64_t commit_id = 2;

    SequentialCCController::ModifyMap changes;
    SetData("0x123", 5000, changes);
    SetData("0x124", 6000, changes);

    controller.PushCommit(commit_id, changes);
  }

  {
    int64_t commit_id = 1;

    SequentialCCController::ModifyMap changes;
    GetData("0x123", storage, changes);
    SetData("0x124", 1000, changes);

    controller.PushCommit(commit_id, changes);
  }

  done_future.get();
  EXPECT_EQ(storage.Load(HexToInt("0x120")).first, 0000);
  EXPECT_EQ(storage.Load(HexToInt("0x121")).first, 1000);
  EXPECT_EQ(storage.Load(HexToInt("0x122")).first, 2000);
  EXPECT_EQ(storage.Load(HexToInt("0x123")).first, 5000);
  EXPECT_EQ(storage.Load(HexToInt("0x124")).first, 6000);
}

TEST(ContractDagManagerTest, PushTwoOOOCommitRARNoRedo) {
  DataStorage storage;
  SequentialCCController controller(&storage);

  storage.Store(HexToInt("0x120"), 0000);
  storage.Store(HexToInt("0x121"), 1000);
  storage.Store(HexToInt("0x122"), 2000);
  storage.Store(HexToInt("0x123"), 3000);
  storage.Store(HexToInt("0x124"), 4000);

  std::promise<bool> done;
  std::future<bool> done_future = done.get_future();

  int committed1_done = 0, committed2_done = 0, redo_num = 0;
  SequentialCCController::CallBack call_back;
  call_back.committed_callback = [&](int64_t commit_id) {
    if (commit_id == 1) {
      committed1_done = 1;
    }
    if (commit_id == 2) {
      committed2_done = 1;
    }
    if (committed1_done == 1 && committed2_done == 1 && redo_num == 0) {
      done.set_value(true);
    }
  };

  call_back.redo_callback = [&](int64_t commit_id) {
    EXPECT_EQ(commit_id, 2);
    redo_num = 1;
    LOG(ERROR) << "redo:" << commit_id;
  };

  controller.SetCallback(call_back);
  {
    int64_t commit_id = 2;

    SequentialCCController::ModifyMap changes;
    GetData("0x123", storage, changes);
    SetData("0x124", 6000, changes);

    controller.PushCommit(commit_id, changes);
  }

  {
    int64_t commit_id = 1;

    SequentialCCController::ModifyMap changes;
    GetData("0x123", storage, changes);
    SetData("0x124", 1000, changes);

    controller.PushCommit(commit_id, changes);
  }

  done_future.get();
  EXPECT_EQ(storage.Load(HexToInt("0x120")).first, 0000);
  EXPECT_EQ(storage.Load(HexToInt("0x121")).first, 1000);
  EXPECT_EQ(storage.Load(HexToInt("0x122")).first, 2000);
  EXPECT_EQ(storage.Load(HexToInt("0x123")).first, 3000);
  EXPECT_EQ(storage.Load(HexToInt("0x124")).first, 6000);
}

TEST(ContractDagManagerTest, ReadAndWriteSameAddress) {
  DataStorage storage;
  SequentialCCController controller(&storage);

  storage.Store(HexToInt("0x120"), 0000);
  storage.Store(HexToInt("0x121"), 1000);
  storage.Store(HexToInt("0x122"), 2000);
  storage.Store(HexToInt("0x123"), 3000);
  storage.Store(HexToInt("0x124"), 4000);

  std::promise<bool> done;
  std::future<bool> done_future = done.get_future();

  int committed1_done = 0, committed2_done = 0, redo_num = 0;
  SequentialCCController::CallBack call_back;
  call_back.committed_callback = [&](int64_t commit_id) {
    LOG(ERROR) << "commit id ====:" << commit_id;
    if (commit_id == 1) {
      committed1_done++;
    }
    if (commit_id == 2) {
      committed2_done++;
    }
    if (committed1_done == 1 && committed2_done == 1 && redo_num >= 1) {
      done.set_value(true);
    }
  };

  call_back.redo_callback = [&](int64_t commit_id) {
    EXPECT_EQ(commit_id, 2);
    redo_num = 1;
    LOG(ERROR) << "redo:" << commit_id;
    SequentialCCController::ModifyMap changes;
    GetData("0x124", storage, changes);
    SetData("0x124", 6000, changes);

    controller.PushCommit(commit_id, changes);
  };

  controller.SetCallback(call_back);
  {
    int64_t commit_id = 2;

    SequentialCCController::ModifyMap changes;
    GetData("0x124", storage, changes);
    SetData("0x124", 6000, changes);

    controller.PushCommit(commit_id, changes);
  }

  {
    int64_t commit_id = 1;

    SequentialCCController::ModifyMap changes;
    GetData("0x124", storage, changes);
    SetData("0x124", 1000, changes);

    controller.PushCommit(commit_id, changes);
  }

  done_future.get();
  EXPECT_EQ(storage.Load(HexToInt("0x124")).first, 6000);
}

}  // namespace
}  // namespace contract
}  // namespace resdb
