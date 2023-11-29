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

#include "platform/consensus/recovery/recovery.h"

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <future>

#include "chain/storage/mock_storage.h"
#include "common/test/test_macros.h"
#include "platform/consensus/checkpoint/mock_checkpoint.h"
#include "platform/consensus/ordering/common/transaction_utils.h"

namespace resdb {
namespace {

using ::resdb::testing::EqualsProto;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::Test;

const std::string log_path = "./log/test_log";

ResConfigData GetConfigData(int buf_size = 10) {
  ResConfigData data;
  data.set_recovery_enabled(true);
  data.set_recovery_path(log_path);
  data.set_recovery_buffer_size(buf_size);
  data.set_recovery_ckpt_time_s(1);

  return data;
}

std::vector<std::string> Listlogs(const std::string &path) {
  std::vector<std::string> ret;
  std::string dir = std::filesystem::path(path).parent_path();
  for (const auto &entry : std::filesystem::directory_iterator(dir)) {
    LOG(ERROR) << "path:" << entry.path();
    ret.push_back(entry.path());
  }
  return ret;
}

class RecoveryTest : public Test {
 public:
  RecoveryTest()
      : config_(GetConfigData(), ReplicaInfo(), KeyInfo(), CertificateInfo()),
        system_info_() {
    std::string dir = std::filesystem::path(log_path).parent_path();
    std::filesystem::remove_all(dir);
  }

 protected:
  ResDBConfig config_;
  SystemInfo system_info_;
  MockCheckPoint checkpoint_;
};

TEST_F(RecoveryTest, ReadLog) {
  std::vector<int> types = {Request::TYPE_PRE_PREPARE, Request::TYPE_PREPARE,
                            Request::TYPE_COMMIT,      Request::TYPE_CHECKPOINT,
                            Request::TYPE_NEWVIEW,     Request::TYPE_NEW_TXNS};

  std::vector<int> expected_types = {
      Request::TYPE_PRE_PREPARE, Request::TYPE_PREPARE, Request::TYPE_COMMIT,
      Request::TYPE_CHECKPOINT,  Request::TYPE_NEWVIEW,
  };

  {
    Recovery recovery(config_, &checkpoint_, &system_info_, nullptr);

    for (int t : types) {
      std::unique_ptr<Request> request =
          NewRequest(static_cast<resdb::Request_Type>(t), Request(), 0);
      request->set_seq(1);
      recovery.AddRequest(nullptr, request.get());
    }
  }
  {
    std::vector<Request> list;
    Recovery recovery(config_, &checkpoint_, &system_info_, nullptr);
    recovery.ReadLogs(
        [&](const SystemInfoData &data) {},
        [&](std::unique_ptr<Context> context,
            std::unique_ptr<Request> request) { list.push_back(*request); });

    EXPECT_EQ(list.size(), expected_types.size());

    for (size_t i = 0; i < expected_types.size(); ++i) {
      EXPECT_EQ(list[i].type(), expected_types[i]);
    }
  }
}

TEST_F(RecoveryTest, ReadLog_FlushOnce) {
  ResDBConfig config(GetConfigData(1024), ReplicaInfo(), KeyInfo(),
                     CertificateInfo());

  std::vector<int> types = {Request::TYPE_PRE_PREPARE, Request::TYPE_PREPARE,
                            Request::TYPE_COMMIT,      Request::TYPE_CHECKPOINT,
                            Request::TYPE_NEWVIEW,     Request::TYPE_NEW_TXNS};

  std::vector<int> expected_types = {
      Request::TYPE_PRE_PREPARE, Request::TYPE_PREPARE, Request::TYPE_COMMIT,
      Request::TYPE_CHECKPOINT,  Request::TYPE_NEWVIEW,
  };

  {
    Recovery recovery(config, &checkpoint_, &system_info_, nullptr);

    for (int t : types) {
      std::unique_ptr<Request> request =
          NewRequest(static_cast<resdb::Request_Type>(t), Request(), 0);
      request->set_seq(1);
      recovery.AddRequest(nullptr, request.get());
    }
  }
  {
    std::vector<Request> list;
    Recovery recovery(config, &checkpoint_, &system_info_, nullptr);
    recovery.ReadLogs([&](const SystemInfoData &data) {},
                      [&](std::unique_ptr<Context> context,
                          std::unique_ptr<Request> request) {
                        LOG(ERROR) << "call back:" << request->seq();
                        list.push_back(*request);
                      });

    EXPECT_EQ(list.size(), expected_types.size());

    for (size_t i = 0; i < expected_types.size(); ++i) {
      EXPECT_EQ(list[i].type(), expected_types[i]);
    }
  }
}

TEST_F(RecoveryTest, CheckPoint) {
  ResDBConfig config(GetConfigData(1024), ReplicaInfo(), KeyInfo(),
                     CertificateInfo());

  std::vector<int> types = {Request::TYPE_PRE_PREPARE, Request::TYPE_PREPARE,
                            Request::TYPE_COMMIT};

  std::vector<int> expected_types = {
      Request::TYPE_PRE_PREPARE, Request::TYPE_PREPARE, Request::TYPE_COMMIT};

  std::promise<bool> insert_done, ckpt;
  std::future<bool> insert_done_future = insert_done.get_future(),
                    ckpt_future = ckpt.get_future();
  int time = 1;
  EXPECT_CALL(checkpoint_, GetStableCheckpoint()).WillRepeatedly(Invoke([&]() {
    if (time == 1) {
      insert_done_future.get();
    } else if (time == 2) {
      ckpt.set_value(true);
    }
    time++;
    return 5;
  }));

  {
    Recovery recovery(config, &checkpoint_, &system_info_, nullptr);

    for (int i = 1; i < 10; ++i) {
      for (int t : types) {
        std::unique_ptr<Request> request =
            NewRequest(static_cast<resdb::Request_Type>(t), Request(), i);
        request->set_seq(i);
        recovery.AddRequest(nullptr, request.get());
      }
    }
    insert_done.set_value(true);
    ckpt_future.get();
    for (int i = 10; i < 20; ++i) {
      for (int t : types) {
        std::unique_ptr<Request> request =
            NewRequest(static_cast<resdb::Request_Type>(t), Request(), i);
        request->set_seq(i);
        recovery.AddRequest(nullptr, request.get());
      }
    }
  }
  std::vector<std::string> log_list = Listlogs(log_path);
  EXPECT_EQ(log_list.size(), 2);
  {
    std::vector<Request> list;
    Recovery recovery(config, &checkpoint_, &system_info_, nullptr);
    recovery.ReadLogs([&](const SystemInfoData &data) {},
                      [&](std::unique_ptr<Context> context,
                          std::unique_ptr<Request> request) {
                        list.push_back(*request);
                        // LOG(ERROR)<<"call back:"<<request->seq();
                      });

    EXPECT_EQ(list.size(), types.size() * 14);

    for (size_t i = 0; i < expected_types.size(); ++i) {
      EXPECT_EQ(list[i].type(), expected_types[i]);
    }
  }
}

TEST_F(RecoveryTest, CheckPoint2) {
  ResDBConfig config(GetConfigData(1024), ReplicaInfo(), KeyInfo(),
                     CertificateInfo());
  MockStorage storage;
  EXPECT_CALL(storage, Flush).Times(2).WillRepeatedly(Return(true));

  std::vector<int> types = {Request::TYPE_PRE_PREPARE, Request::TYPE_PREPARE,
                            Request::TYPE_COMMIT};

  std::vector<int> expected_types = {
      Request::TYPE_PRE_PREPARE, Request::TYPE_PREPARE, Request::TYPE_COMMIT};

  std::promise<bool> insert_done, ckpt, insert_done2, ckpt2;
  std::future<bool> insert_done_future = insert_done.get_future(),
                    ckpt_future = ckpt.get_future();
  std::future<bool> insert_done2_future = insert_done2.get_future();
  std::future<bool> ckpt_future2 = ckpt2.get_future();
  int time = 1;
  EXPECT_CALL(checkpoint_, GetStableCheckpoint()).WillRepeatedly(Invoke([&]() {
    if (time == 1) {
      insert_done_future.get();
    } else if (time == 2) {
      ckpt.set_value(true);
    } else if (time == 3) {
      insert_done2_future.get();
    } else if (time == 4) {
      ckpt2.set_value(true);
    }
    time++;
    if (time > 3) {
      return 25;
    }
    return 5;
  }));

  {
    Recovery recovery(config, &checkpoint_, &system_info_, &storage);

    for (int i = 1; i < 10; ++i) {
      for (int t : types) {
        std::unique_ptr<Request> request =
            NewRequest(static_cast<resdb::Request_Type>(t), Request(), i);
        request->set_seq(i);
        recovery.AddRequest(nullptr, request.get());
      }
    }
    insert_done.set_value(true);
    ckpt_future.get();
    for (int i = 10; i < 20; ++i) {
      for (int t : types) {
        std::unique_ptr<Request> request =
            NewRequest(static_cast<resdb::Request_Type>(t), Request(), i);
        request->set_seq(i);
        recovery.AddRequest(nullptr, request.get());
      }
    }
  }
  std::vector<std::string> log_list = Listlogs(log_path);
  EXPECT_EQ(log_list.size(), 2);
  {
    std::vector<Request> list;
    Recovery recovery(config, &checkpoint_, &system_info_, &storage);
    recovery.ReadLogs([&](const SystemInfoData &data) {},
                      [&](std::unique_ptr<Context> context,
                          std::unique_ptr<Request> request) {
                        list.push_back(*request);
                        // LOG(ERROR)<<"call back:"<<request->seq();
                      });

    EXPECT_EQ(list.size(), types.size() * 14);

    for (size_t i = 0; i < expected_types.size(); ++i) {
      EXPECT_EQ(list[i].type(), expected_types[i]);
    }

    for (int i = 20; i < 30; ++i) {
      for (int t : types) {
        std::unique_ptr<Request> request =
            NewRequest(static_cast<resdb::Request_Type>(t), Request(), i);
        request->set_seq(i);
        recovery.AddRequest(nullptr, request.get());
      }
    }
    insert_done2.set_value(true);
    ckpt_future2.get();

    for (int i = 30; i < 35; ++i) {
      for (int t : types) {
        std::unique_ptr<Request> request =
            NewRequest(static_cast<resdb::Request_Type>(t), Request(), i);
        request->set_seq(i);
        recovery.AddRequest(nullptr, request.get());
      }
    }
  }

  {
    std::vector<Request> list;
    Recovery recovery(config, &checkpoint_, &system_info_, &storage);
    recovery.ReadLogs([&](const SystemInfoData &data) {},
                      [&](std::unique_ptr<Context> context,
                          std::unique_ptr<Request> request) {
                        list.push_back(*request);
                        // LOG(ERROR)<<"call back:"<<request->seq();
                      });

    EXPECT_EQ(list.size(), types.size() * 9);

    for (size_t i = 0; i < expected_types.size(); ++i) {
      EXPECT_EQ(list[i].type(), expected_types[i]);
    }
    EXPECT_EQ(recovery.GetMinSeq(), 30);
    EXPECT_EQ(recovery.GetMaxSeq(), 34);
  }
}

TEST_F(RecoveryTest, SystemInfo) {
  ResDBConfig config(GetConfigData(1024), ReplicaInfo(), KeyInfo(),
                     CertificateInfo());
  MockStorage storage;
  EXPECT_CALL(storage, Flush).Times(2).WillRepeatedly(Return(true));

  std::vector<int> types = {Request::TYPE_PRE_PREPARE, Request::TYPE_PREPARE,
                            Request::TYPE_COMMIT};

  std::vector<int> expected_types = {
      Request::TYPE_PRE_PREPARE, Request::TYPE_PREPARE, Request::TYPE_COMMIT};

  std::promise<bool> insert_done, ckpt, insert_done2, ckpt2;
  std::future<bool> insert_done_future = insert_done.get_future(),
                    ckpt_future = ckpt.get_future();
  std::future<bool> insert_done2_future = insert_done2.get_future();
  std::future<bool> ckpt_future2 = ckpt2.get_future();
  int time = 1;
  EXPECT_CALL(checkpoint_, GetStableCheckpoint()).WillRepeatedly(Invoke([&]() {
    if (time == 1) {
      insert_done_future.get();
    } else if (time == 2) {
      ckpt.set_value(true);
    } else if (time == 3) {
      insert_done2_future.get();
    } else if (time == 4) {
      ckpt2.set_value(true);
    }
    time++;
    if (time > 3) {
      return 25;
    }
    return 5;
  }));

  {
    Recovery recovery(config, &checkpoint_, &system_info_, &storage);
    system_info_.SetCurrentView(2);
    system_info_.SetPrimary(2);

    for (int i = 1; i < 10; ++i) {
      for (int t : types) {
        std::unique_ptr<Request> request =
            NewRequest(static_cast<resdb::Request_Type>(t), Request(), i);
        request->set_seq(i);
        recovery.AddRequest(nullptr, request.get());
      }
    }
    insert_done.set_value(true);
    ckpt_future.get();
    for (int i = 10; i < 20; ++i) {
      for (int t : types) {
        std::unique_ptr<Request> request =
            NewRequest(static_cast<resdb::Request_Type>(t), Request(), i);
        request->set_seq(i);
        recovery.AddRequest(nullptr, request.get());
      }
    }
  }
  std::vector<std::string> log_list = Listlogs(log_path);
  EXPECT_EQ(log_list.size(), 2);
  {
    std::vector<Request> list;
    SystemInfoData data;
    Recovery recovery(config, &checkpoint_, &system_info_, &storage);
    recovery.ReadLogs([&](const SystemInfoData &r_data) { data = r_data; },
                      [&](std::unique_ptr<Context> context,
                          std::unique_ptr<Request> request) {
                        list.push_back(*request);
                        // LOG(ERROR)<<"call back:"<<request->seq();
                      });

    EXPECT_EQ(list.size(), types.size() * 14);

    for (size_t i = 0; i < expected_types.size(); ++i) {
      EXPECT_EQ(list[i].type(), expected_types[i]);
    }

    for (int i = 20; i < 30; ++i) {
      for (int t : types) {
        std::unique_ptr<Request> request =
            NewRequest(static_cast<resdb::Request_Type>(t), Request(), i);
        request->set_seq(i);
        recovery.AddRequest(nullptr, request.get());
      }
    }
    insert_done2.set_value(true);
    ckpt_future2.get();

    for (int i = 30; i < 35; ++i) {
      for (int t : types) {
        std::unique_ptr<Request> request =
            NewRequest(static_cast<resdb::Request_Type>(t), Request(), i);
        request->set_seq(i);
        recovery.AddRequest(nullptr, request.get());
      }
    }
  }

  {
    std::vector<Request> list;
    SystemInfoData data;
    Recovery recovery(config, &checkpoint_, &system_info_, &storage);
    recovery.ReadLogs([&](const SystemInfoData &r_data) { data = r_data; },
                      [&](std::unique_ptr<Context> context,
                          std::unique_ptr<Request> request) {
                        list.push_back(*request);
                        // LOG(ERROR)<<"call back:"<<request->seq();
                      });

    EXPECT_EQ(data.view(), 2);
    EXPECT_EQ(data.primary_id(), 2);
    EXPECT_EQ(list.size(), types.size() * 9);

    for (size_t i = 0; i < expected_types.size(); ++i) {
      EXPECT_EQ(list[i].type(), expected_types[i]);
    }
    EXPECT_EQ(recovery.GetMinSeq(), 30);
    EXPECT_EQ(recovery.GetMaxSeq(), 34);
  }
}

}  // namespace

}  // namespace resdb
