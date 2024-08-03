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

#include "platform/consensus/ordering/pbft/checkpoint_manager.h"

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <future>

#include "common/crypto/mock_signature_verifier.h"
#include "common/test/test_macros.h"
#include "platform/config/resdb_config_utils.h"
#include "platform/consensus/ordering/pbft/transaction_utils.h"
#include "platform/networkstrate/mock_replica_communicator.h"
#include "platform/proto/checkpoint_info.pb.h"
#include "platform/statistic/stats.h"

namespace resdb {
namespace {

using ::resdb::testing::EqualsProto;
using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::Test;

class MyCheckPointManager : public CheckPointManager {
 public:
  MyCheckPointManager(const ResDBConfig& config,
                      ReplicaCommunicator* replica_communicator,
                      SignatureVerifier* verifier,
                      std::function<void(int64_t)> call_back = nullptr)
      : CheckPointManager(config, replica_communicator, verifier),
        call_back_(call_back) {}

  void UpdateStableCheckPointCallback(int64_t stable_checkpoint) {
    if (call_back_) {
      call_back_(stable_checkpoint);
    }
  }
  std::function<void(int64_t)> call_back_;
};

ResConfigData GetConfigData() {
  Stats::GetGlobalStats(/*int sleep_seconds = */ 1);
  std::string json =
      "{ "
      " \"region\":{ "
      "  \"replica_info\": { "
      "  \"id\": 1, "
      "  \"ip\": \"127.0.0.1\", "
      "  \"port\": 1234 "
      "   },"
      "  \"replica_info\": { "
      "  \"id\": 2, "
      "  \"ip\": \"127.0.0.1\", "
      "  \"port\": 1235 "
      "   },"
      "  \"replica_info\": { "
      "  \"id\": 3, "
      "  \"ip\": \"127.0.0.1\", "
      "  \"port\": 1236 "
      "   },"
      "  \"replica_info\": { "
      "  \"id\": 4, "
      "  \"ip\": \"127.0.0.1\", "
      "  \"port\": 1237 "
      "   },"
      " },"
      "\"view_change_timeout_ms\": 500"
      "}";
  return resdb::testing::ParseFromText<ResConfigData>(json);
}

class CheckPointManagerTest : public Test {
 public:
  CheckPointManagerTest()
      :  // just set the monitor time to 1 second to return early.
        config_(GetConfigData(), GenerateReplicaInfo(1, "127.0.0.1", 1234),
                KeyInfo(), CertificateInfo()) {
    config_.EnableCheckPoint(true);
  }

 protected:
  ResDBConfig config_;
  MockReplicaCommunicator replica_communicator_;
};

TEST_F(CheckPointManagerTest, SendCheckPoint) {
  config_.SetViewchangeCommitTimeout(100);
  CheckPointManager manager(config_, &replica_communicator_, nullptr);

  for (int i = 1; i <= 5; ++i) {
    std::unique_ptr<Request> request = std::make_unique<Request>();
    request->set_seq(i);
    manager.AddCommitData(std::move(request));
  }

  std::promise<bool> propose_done;
  std::future<bool> propose_done_future = propose_done.get_future();
  EXPECT_CALL(replica_communicator_, BroadCast).WillOnce(Invoke([&]() {
    LOG(ERROR) << "bcbcbc";
    propose_done.set_value(true);
  }));
  propose_done_future.get();
  LOG(ERROR) << "done";
}

TEST_F(CheckPointManagerTest, SendCheckPointOnce) {
  std::promise<bool> propose_done;
  std::future<bool> propose_done_future = propose_done.get_future();
  EXPECT_CALL(replica_communicator_, BroadCast).WillOnce(Invoke([&]() {
    LOG(ERROR) << "bc:";
    propose_done.set_value(true);
  }));

  CheckPointManager manager(config_, &replica_communicator_, nullptr);
  for (int i = 1; i <= 5; ++i) {
    std::unique_ptr<Request> request = std::make_unique<Request>();
    request->set_seq(i);
    manager.AddCommitData(std::move(request));
  }
  sleep(1);
  {
    std::unique_ptr<Request> request = std::make_unique<Request>();
    request->set_seq(6);
    manager.AddCommitData(std::move(request));
  }

  propose_done_future.get();
}

TEST_F(CheckPointManagerTest, SendCheckPointTwo) {
  config_.SetViewchangeCommitTimeout(100);
  std::promise<bool> propose_done;
  std::future<bool> propose_done_future = propose_done.get_future();
  int count = 0;
  EXPECT_CALL(replica_communicator_, BroadCast).WillRepeatedly(Invoke([&]() {
    if (++count == 2) {
      propose_done.set_value(true);
    }
  }));

  CheckPointManager manager(config_, &replica_communicator_, nullptr);
  std::unique_ptr<Request> request = std::make_unique<Request>();
  for (int i = 1; i <= 5; ++i) {
    std::unique_ptr<Request> request = std::make_unique<Request>();
    request->set_seq(i);
    manager.AddCommitData(std::move(request));
  }
  sleep(1);
  for (int i = 6; i <= 11; ++i) {
    std::unique_ptr<Request> request = std::make_unique<Request>();
    request->set_seq(i);
    manager.AddCommitData(std::move(request));
  }

  propose_done_future.get();
}

TEST_F(CheckPointManagerTest, StableCkpt) {
  config_.SetViewchangeCommitTimeout(100);
  std::promise<bool> ckp_done;
  std::future<bool> ckp_done_future = ckp_done.get_future();
  bool already_set = 0;
  MyCheckPointManager manager(config_, &replica_communicator_, nullptr,
                              [&](int64_t seq) {
                                if (seq > 0 && !already_set) {
                                  already_set = true;
                                  ckp_done.set_value(true);
                                }
                              });
  for (int i = 1; i <= 4; ++i) {
    CheckPointData checkpoint_data;
    std::unique_ptr<Request> checkpoint_request =
        NewRequest(Request::TYPE_CHECKPOINT, Request(), i);
    checkpoint_data.set_seq(5);
    checkpoint_data.set_hash("1234");
    checkpoint_data.SerializeToString(checkpoint_request->mutable_data());
    EXPECT_EQ(manager.ProcessCheckPoint(std::make_unique<Context>(),
                                        std::move(checkpoint_request)),
              0);
  }
  ckp_done_future.get();
  EXPECT_EQ(manager.GetStableCheckpoint(), 5);
}

TEST_F(CheckPointManagerTest, StableCkptNotEnough) {
  config_.SetViewchangeCommitTimeout(1000);
  std::promise<bool> ckp_done;
  std::future<bool> ckp_done_future = ckp_done.get_future();
  bool set_done = false;
  MyCheckPointManager manager(config_, &replica_communicator_, nullptr,
                              [&](int64_t seq) {
                                if (seq > 0 && !set_done) {
                                  set_done = true;
                                  ckp_done.set_value(true);
                                }
                              });
  for (int i = 1; i <= 4; ++i) {
    CheckPointData checkpoint_data;
    std::unique_ptr<Request> checkpoint_request =
        NewRequest(Request::TYPE_CHECKPOINT, Request(), i);
    checkpoint_data.set_seq(5);
    checkpoint_data.set_hash("1234");
    checkpoint_data.SerializeToString(checkpoint_request->mutable_data());
    EXPECT_EQ(manager.ProcessCheckPoint(std::make_unique<Context>(),
                                        std::move(checkpoint_request)),
              0);
  }
  for (int i = 1; i <= 4; ++i) {
    CheckPointData checkpoint_data;
    std::unique_ptr<Request> checkpoint_request =
        NewRequest(Request::TYPE_CHECKPOINT, Request(), i);
    checkpoint_data.set_seq(10);
    if (i <= 2) {
      checkpoint_data.set_hash("1234");
    } else {
      checkpoint_data.set_hash("2345");
    }
    checkpoint_data.SerializeToString(checkpoint_request->mutable_data());
    EXPECT_EQ(manager.ProcessCheckPoint(std::make_unique<Context>(),
                                        std::move(checkpoint_request)),
              0);
  }

  ckp_done_future.get();
  EXPECT_EQ(manager.GetStableCheckpoint(), 5);
}

TEST_F(CheckPointManagerTest, Votes) {
  config_.SetViewchangeCommitTimeout(100);
  MockSignatureVerifier mock_verifier;
  EXPECT_CALL(mock_verifier, SignMessage).WillOnce(Return(SignatureInfo()));
  EXPECT_CALL(mock_verifier, VerifyMessage(_, EqualsProto(SignatureInfo())))
      .WillRepeatedly(Return(true));

  std::promise<bool> propose_done;
  std::future<bool> propose_done_future = propose_done.get_future();
  CheckPointManager manager(config_, &replica_communicator_, &mock_verifier);
  EXPECT_CALL(replica_communicator_, BroadCast)
      .WillRepeatedly(Invoke([&](const google::protobuf::Message& message) {
        for (int i = 1; i <= 3; ++i) {
          std::unique_ptr<Request> checkpoint_request =
              std::make_unique<Request>();
          checkpoint_request->CopyFrom(message);
          checkpoint_request->set_sender_id(i);
          EXPECT_EQ(manager.ProcessCheckPoint(std::make_unique<Context>(),
                                              std::move(checkpoint_request)),
                    0);
        }
        propose_done.set_value(true);
      }));

  std::unique_ptr<Request> request = std::make_unique<Request>();
  for (int i = 1; i <= 5; ++i) {
    std::unique_ptr<Request> request = std::make_unique<Request>();
    request->set_seq(i);
    manager.AddCommitData(std::move(request));
  }
  propose_done_future.get();
  sleep(1);
  StableCheckPoint ckpt = manager.GetStableCheckpointWithVotes();
  EXPECT_EQ(ckpt.seq(), 5);
  EXPECT_EQ(ckpt.signatures_size(), 3);
}

/*
TEST_F(CheckPointManagerTest, SetTimeoutHandler) {
  CheckPointManager manager(config_, &replica_communicator_, nullptr);

  std::promise<bool> propose_done;
  std::future<bool> propose_done_future = propose_done.get_future();
  int ct = 0;
  manager.SetTimeoutHandler([&]() {
    if (ct++ == 0) {
      propose_done.set_value(true);
    }
  });
  std::unique_ptr<Request> request = std::make_unique<Request>();
  request->set_seq(1);
  manager.AddCommitData(std::move(request));
  propose_done_future.get();
}
*/

}  // namespace

}  // namespace resdb
