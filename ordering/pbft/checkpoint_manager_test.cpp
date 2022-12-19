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

#include "ordering/pbft/checkpoint_manager.h"

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <future>

#include "common/test/test_macros.h"
#include "config/resdb_config_utils.h"
#include "crypto/mock_signature_verifier.h"
#include "ordering/pbft/transaction_utils.h"
#include "proto/checkpoint_info.pb.h"
#include "server/mock_resdb_replica_client.h"
#include "statistic/stats.h"

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
                      ResDBReplicaClient* replica_client,
                      SignatureVerifier* verifier,
                      std::function<void(int64_t)> call_back = nullptr)
      : CheckPointManager(config, replica_client, verifier),
        call_back_(call_back) {}

  void UpdateStableCheckPointCallback(int64_t stable_checkpoint) {
    if (call_back_) {
      call_back_(stable_checkpoint);
    }
  }
  std::function<void(int64_t)> call_back_;
};

class CheckPointManagerTest : public Test {
 public:
  CheckPointManagerTest()
      :  // just set the monitor time to 1 second to return early.
        config_({GenerateReplicaInfo(1, "127.0.0.1", 1234),
                 GenerateReplicaInfo(2, "127.0.0.1", 1235),
                 GenerateReplicaInfo(3, "127.0.0.1", 1236),
                 GenerateReplicaInfo(4, "127.0.0.1", 1237)},
                GenerateReplicaInfo(1, "127.0.0.1", 1234)) {
    config_.EnableCheckPoint(true);
    Stats::GetGlobalStats(/*int sleep_seconds = */ 1);
  }

 protected:
  MockResDBReplicaClient replica_client_;
  ResDBConfig config_;
};

TEST_F(CheckPointManagerTest, SendCheckPoint) {
  config_.SetViewchangeCommitTimeout(100);
  CheckPointManager manager(config_, &replica_client_, nullptr);

  for (int i = 1; i <= 5; ++i) {
    std::unique_ptr<Request> request = std::make_unique<Request>();
    request->set_seq(i);
    manager.AddCommitData(std::move(request));
  }

  std::promise<bool> propose_done;
  std::future<bool> propose_done_future = propose_done.get_future();
  EXPECT_CALL(replica_client_, BroadCast).WillOnce(Invoke([&]() {
    LOG(ERROR) << "bcbcbc";
    propose_done.set_value(true);
  }));
  propose_done_future.get();
  LOG(ERROR) << "done";
}

TEST_F(CheckPointManagerTest, SendCheckPointOnce) {
  std::promise<bool> propose_done;
  std::future<bool> propose_done_future = propose_done.get_future();
  EXPECT_CALL(replica_client_, BroadCast).WillOnce(Invoke([&]() {
    LOG(ERROR) << "bc:";
    propose_done.set_value(true);
  }));

  CheckPointManager manager(config_, &replica_client_, nullptr);
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
  EXPECT_CALL(replica_client_, BroadCast).WillRepeatedly(Invoke([&]() {
    LOG(ERROR) << "bc:";
    if (++count == 2) {
      propose_done.set_value(true);
    }
  }));

  CheckPointManager manager(config_, &replica_client_, nullptr);
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
  MyCheckPointManager manager(config_, &replica_client_, nullptr,
                              [&](int64_t seq) {
                                if (seq > 0) {
                                  ckp_done.set_value(true);
                                }
                              });
  sleep(1);
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

TEST_F(CheckPointManagerTest, StableCkptNotEnought) {
  config_.SetViewchangeCommitTimeout(1000);
  std::promise<bool> ckp_done;
  std::future<bool> ckp_done_future = ckp_done.get_future();
  MyCheckPointManager manager(config_, &replica_client_, nullptr,
                              [&](int64_t seq) {
                                if (seq > 0) {
                                  ckp_done.set_value(true);
                                }
                              });
  sleep(2);
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
  CheckPointManager manager(config_, &replica_client_, &mock_verifier);
  EXPECT_CALL(replica_client_, BroadCast)
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

TEST_F(CheckPointManagerTest, SetTimeoutHandler) {
  CheckPointManager manager(config_, &replica_client_, nullptr);

  std::promise<bool> propose_done;
  std::future<bool> propose_done_future = propose_done.get_future();
  int ct = 0;
  manager.SetTimeoutHandler([&]() {
    if (ct++ == 0) {
      propose_done.set_value(true);
    }
  });
  propose_done_future.get();
}

}  // namespace

}  // namespace resdb
