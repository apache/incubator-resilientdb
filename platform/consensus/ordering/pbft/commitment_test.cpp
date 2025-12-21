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

#include "platform/consensus/ordering/pbft/commitment.h"

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <future>

#include "common/crypto/mock_signature_verifier.h"
#include "common/test/test_macros.h"
#include "interface/rdbc/mock_net_channel.h"
#include "platform/config/resdb_config_utils.h"
#include "platform/consensus/ordering/pbft/checkpoint_manager.h"
#include "platform/consensus/ordering/pbft/message_manager.h"
#include "platform/networkstrate/mock_replica_communicator.h"

namespace resdb {
namespace {

using ::resdb::testing::EqualsProto;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::Test;

ResDBConfig GenerateConfig() {
  ResConfigData data;
  data.set_duplicate_check_frequency_useconds(100000);
  return ResDBConfig({GenerateReplicaInfo(1, "127.0.0.1", 1234),
                      GenerateReplicaInfo(2, "127.0.0.1", 1235),
                      GenerateReplicaInfo(3, "127.0.0.1", 1236),
                      GenerateReplicaInfo(4, "127.0.0.1", 1237)},
                     GenerateReplicaInfo(1, "127.0.0.1", 1234), data);
}

class CommitmentTest : public Test {
 public:
  CommitmentTest()
      :  // just set the monitor time to 1 second to return early.
        global_stats_(Stats::GetGlobalStats(1)),
        config_(GenerateConfig()),
        system_info_(config_),
        checkpoint_manager_(config_, &replica_communicator_, &verifier_),
        message_manager_(std::make_unique<MessageManager>(
            config_, nullptr, &checkpoint_manager_, &system_info_)),
        commitment_(
            std::make_unique<Commitment>(config_, message_manager_.get(),
                                         &replica_communicator_, &verifier_)) {}

  std::unique_ptr<Context> GetContext() {
    auto context = std::make_unique<Context>();
    context->signature.set_signature("signature");
    return context;
  }

  int AddProposeMsg(int sender_id, bool need_resp = false, int proxy_id = 1) {
    auto context = std::make_unique<Context>();
    context->signature.set_signature("signature");

    Request request;
    request.set_current_view(1);
    request.set_seq(1);
    request.set_type(Request::TYPE_PRE_PREPARE);
    request.set_sender_id(sender_id);
    request.set_need_response(need_resp);
    request.set_proxy_id(proxy_id);
    request.set_data(data_);

    return commitment_->ProcessProposeMsg(std::move(context),
                                          std::make_unique<Request>(request));
  }

  int AddPrepareMsg(int sender_id) {
    auto context = std::make_unique<Context>();
    context->signature.set_signature("signature");

    Request request;
    request.set_current_view(1);
    request.set_seq(1);
    request.set_type(Request::TYPE_PREPARE);
    request.set_sender_id(sender_id);
    return commitment_->ProcessPrepareMsg(std::move(context),
                                          std::make_unique<Request>(request));
  }

  int AddCommitMsg(int sender_id) {
    auto context = std::make_unique<Context>();
    context->signature.set_signature("signature");

    Request request;
    request.set_current_view(1);
    request.set_seq(1);
    request.set_type(Request::TYPE_COMMIT);
    request.set_sender_id(sender_id);
    return commitment_->ProcessCommitMsg(std::move(context),
                                         std::make_unique<Request>(request));
  }

 protected:
  Stats* global_stats_;
  ResDBConfig config_;
  SystemInfo system_info_;
  CheckPointManager checkpoint_manager_;
  MockReplicaCommunicator replica_communicator_;
  MockSignatureVerifier verifier_;
  std::unique_ptr<MessageManager> message_manager_;
  std::unique_ptr<Commitment> commitment_;
  std::string data_;
};

TEST_F(CommitmentTest, NotContesxt) {
  EXPECT_EQ(
      commitment_->ProcessProposeMsg(nullptr, std::make_unique<Request>()), -2);
  EXPECT_EQ(
      commitment_->ProcessPrepareMsg(nullptr, std::make_unique<Request>()), -2);
  EXPECT_EQ(commitment_->ProcessCommitMsg(nullptr, std::make_unique<Request>()),
            -2);
}

TEST_F(CommitmentTest, NoSignature) {
  EXPECT_EQ(commitment_->ProcessProposeMsg(std::make_unique<Context>(),
                                           std::make_unique<Request>()),
            -2);
  EXPECT_EQ(commitment_->ProcessPrepareMsg(std::make_unique<Context>(),
                                           std::make_unique<Request>()),
            -2);
  EXPECT_EQ(commitment_->ProcessCommitMsg(std::make_unique<Context>(),
                                          std::make_unique<Request>()),
            -2);
}

TEST_F(CommitmentTest, NoPrimary) {
  system_info_.SetPrimary(3);
  EXPECT_EQ(AddProposeMsg(Request::TYPE_PRE_PREPARE, 1), -2);
}

TEST_F(CommitmentTest, NewRequest) {
  auto context = std::make_unique<Context>();
  context->signature.set_signature("signature");
  Request request;
  request.set_data("data");

  std::promise<bool> propose_done;
  std::future<bool> propose_done_future = propose_done.get_future();
  EXPECT_CALL(replica_communicator_, BroadCast).WillOnce(Invoke([&]() {
    propose_done.set_value(true);
  }));

  EXPECT_CALL(verifier_,
              VerifyMessage(::testing::_, EqualsProto(SignatureInfo())))
      .WillOnce(Return(true));

  EXPECT_EQ(commitment_->ProcessNewRequest(std::move(context),
                                           std::make_unique<Request>(request)),
            0);
  propose_done_future.get();
}

TEST_F(CommitmentTest, SeqConsumeAll) {
  config_.SetMaxProcessTxn(2);
  commitment_ = nullptr;
  message_manager_ = std::make_unique<MessageManager>(
      config_, nullptr, &checkpoint_manager_, &system_info_);
  commitment_ = std::make_unique<Commitment>(
      config_, message_manager_.get(), &replica_communicator_, &verifier_);
  std::promise<bool> done;
  std::future<bool> done_future = done.get_future();
  EXPECT_CALL(verifier_,
              VerifyMessage(::testing::_, EqualsProto(SignatureInfo())))
      .WillRepeatedly(Return(true));
  for (int i = 0; i < 3; ++i) {
    std::unique_ptr<MockNetChannel> channel =
        std::make_unique<MockNetChannel>("127.0.0.1", 0);
    auto context = std::make_unique<Context>();
    context->signature.set_signature("signature");
    Request request;
    request.set_data("sig" + std::to_string(i));
    request.set_hash("hash" + std::to_string(i));
    if (i < 2) {
      EXPECT_EQ(commitment_->ProcessNewRequest(
                    std::move(context), std::make_unique<Request>(request)),
                0);
    } else {
      EXPECT_EQ(commitment_->ProcessNewRequest(
                    std::move(context), std::make_unique<Request>(request)),
                -2);
    }
  }
}

TEST_F(CommitmentTest, ProposeMsgWithoutSignature) {
  system_info_.SetPrimary(3);
  EXPECT_CALL(replica_communicator_, BroadCast).Times(0);
  EXPECT_CALL(verifier_,
              VerifyMessage(::testing::_, EqualsProto(SignatureInfo())))
      .WillOnce(Return(false));
  EXPECT_EQ(AddProposeMsg(Request::TYPE_PRE_PREPARE, 1), -2);
}

TEST_F(CommitmentTest, ProposeMsg) {
  system_info_.SetPrimary(3);
  BatchUserRequest request;
  request.SerializeToString(&data_);
  EXPECT_CALL(replica_communicator_, BroadCast).Times(1);
  EXPECT_CALL(verifier_, VerifyMessage(data_, EqualsProto(SignatureInfo())))
      .WillOnce(Return(true));

  EXPECT_EQ(AddProposeMsg(Request::TYPE_PRE_PREPARE, 1), 0);
}

TEST_F(CommitmentTest, ProposeMsgOnlyBCOnce) {
  system_info_.SetPrimary(3);
  BatchUserRequest request;
  request.SerializeToString(&data_);
  EXPECT_CALL(replica_communicator_, BroadCast).Times(1);
  EXPECT_CALL(verifier_, VerifyMessage(data_, EqualsProto(SignatureInfo())))
      .WillRepeatedly(Return(true));

  EXPECT_EQ(AddProposeMsg(Request::TYPE_PRE_PREPARE, 1), 0);
  EXPECT_EQ(AddProposeMsg(Request::TYPE_PRE_PREPARE, 2), -2);
}

TEST_F(CommitmentTest, ProcessPrepareMsg) {
  EXPECT_CALL(replica_communicator_, BroadCast).Times(2);

  EXPECT_EQ(AddProposeMsg(1), 0);

  EXPECT_EQ(AddPrepareMsg(1), 0);
  EXPECT_EQ(AddPrepareMsg(2), 0);
  EXPECT_EQ(AddPrepareMsg(3), 0);
}

TEST_F(CommitmentTest, ProcessPrepareMsgNotEnough) {
  EXPECT_CALL(replica_communicator_, BroadCast).Times(1);

  EXPECT_EQ(AddProposeMsg(1), 0);

  EXPECT_EQ(AddPrepareMsg(1), 0);
}

TEST_F(CommitmentTest, ProcessPrepareMsgNoProposeMsg) {
  EXPECT_CALL(replica_communicator_, BroadCast).Times(0);

  EXPECT_EQ(AddPrepareMsg(1), 0);
  EXPECT_EQ(AddPrepareMsg(2), 0);
  EXPECT_EQ(AddPrepareMsg(3), 0);
}

TEST_F(CommitmentTest, ProcessPrepareMsgProposeMsgDelay) {
  EXPECT_CALL(replica_communicator_, BroadCast).Times(2);
  EXPECT_EQ(AddPrepareMsg(2), 0);
  EXPECT_EQ(AddPrepareMsg(3), 0);

  EXPECT_EQ(AddProposeMsg(1), 0);

  EXPECT_EQ(AddPrepareMsg(1), 0);
}

TEST_F(CommitmentTest, ProcessCommitMsg) {
  EXPECT_CALL(replica_communicator_, BroadCast)
      .Times(2);  // 1 propose + 1 prepare

  EXPECT_EQ(AddProposeMsg(1), 0);

  EXPECT_EQ(AddPrepareMsg(1), 0);
  EXPECT_EQ(AddPrepareMsg(2), 0);
  EXPECT_EQ(AddPrepareMsg(3), 0);

  EXPECT_EQ(AddCommitMsg(1), 0);
  EXPECT_EQ(AddCommitMsg(2), 0);
  EXPECT_EQ(AddCommitMsg(3), 0);
}

TEST_F(CommitmentTest, ProcessCommitMsgProposeDelay) {
  EXPECT_CALL(replica_communicator_, BroadCast)
      .Times(2);  // 1 propose + 1 prepare

  EXPECT_EQ(AddPrepareMsg(2), 0);
  EXPECT_EQ(AddPrepareMsg(3), 0);

  EXPECT_EQ(AddCommitMsg(2), 0);
  EXPECT_EQ(AddCommitMsg(3), 0);

  EXPECT_EQ(AddProposeMsg(1), 0);
  EXPECT_EQ(AddPrepareMsg(1), 0);

  EXPECT_EQ(AddCommitMsg(1), 0);
}

TEST_F(CommitmentTest, ProcessCommitMsgWithDuplicated) {
  EXPECT_CALL(replica_communicator_, BroadCast)
      .Times(2);  // 1 propose + 1 prepare

  EXPECT_EQ(AddProposeMsg(1), 0);
  EXPECT_EQ(AddProposeMsg(1), -2);

  EXPECT_EQ(AddPrepareMsg(1), 0);
  EXPECT_EQ(AddPrepareMsg(2), 0);
  EXPECT_EQ(AddPrepareMsg(2), 0);
  EXPECT_EQ(AddPrepareMsg(3), 0);

  EXPECT_EQ(AddCommitMsg(1), 0);
  EXPECT_EQ(AddCommitMsg(2), 0);
  EXPECT_EQ(AddCommitMsg(3), 0);
  EXPECT_EQ(AddCommitMsg(3), -2);
}

TEST_F(CommitmentTest, ProcessCommitMsgWithResponse) {
  EXPECT_CALL(replica_communicator_, BroadCast).Times(2);

  std::promise<bool> done;
  std::future<bool> done_future = done.get_future();

  BatchUserResponse batch_resp;

  batch_resp.set_primary_id(1);
  batch_resp.set_proxy_id(1);
  batch_resp.set_seq(1);
  batch_resp.set_current_view(1);

  // Response Msg
  Request resp_request;
  resp_request.set_type(Request::TYPE_RESPONSE);
  resp_request.set_sender_id(1);
  resp_request.set_current_view(1);
  resp_request.set_seq(1);
  resp_request.set_proxy_id(1);
  resp_request.set_primary_id(1);
  batch_resp.SerializeToString(resp_request.mutable_data());

  EXPECT_CALL(replica_communicator_, SendMessage(EqualsProto(resp_request), 1))
      .WillOnce(Invoke(
          [&](const google::protobuf::Message& request, int64_t node_id) {
            done.set_value(true);
            return 0;
          }));

  EXPECT_EQ(AddProposeMsg(1, true), 0);

  EXPECT_EQ(AddPrepareMsg(1), 0);
  EXPECT_EQ(AddPrepareMsg(2), 0);
  EXPECT_EQ(AddPrepareMsg(3), 0);

  EXPECT_EQ(AddCommitMsg(1), 0);
  EXPECT_EQ(AddCommitMsg(2), 0);
  EXPECT_EQ(AddCommitMsg(3), 0);
  done_future.get();
}

}  // namespace

}  // namespace resdb
