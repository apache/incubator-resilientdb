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

#include "platform/consensus/ordering/pbft/viewchange_manager.h"

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <future>

#include "common/crypto/mock_signature_verifier.h"
#include "common/test/test_macros.h"
#include "platform/config/resdb_config_utils.h"
#include "platform/consensus/execution/system_info.h"
#include "platform/consensus/ordering/pbft/checkpoint_manager.h"
#include "platform/consensus/ordering/pbft/transaction_utils.h"
#include "platform/networkstrate/mock_replica_communicator.h"
#include "platform/proto/checkpoint_info.pb.h"

namespace resdb {
namespace {

using ::resdb::testing::EqualsProto;
using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::Test;

class ViewChangeManagerTest : public Test {
 public:
  ViewChangeManagerTest()
      :  // just set the monitor time to 1 second to return early.
        config_({GenerateReplicaInfo(1, "127.0.0.1", 1234),
                 GenerateReplicaInfo(2, "127.0.0.1", 1235),
                 GenerateReplicaInfo(3, "127.0.0.1", 1236),
                 GenerateReplicaInfo(4, "127.0.0.1", 1237)},
                GenerateReplicaInfo(3, "127.0.0.1", 1234)),
        system_info_(config_) {
    config_.EnableCheckPoint(true);
    config_.SetViewchangeCommitTimeout(1000);  // set to 1s
    checkpoint_manager_ = std::make_unique<CheckPointManager>(
        config_, &replica_communicator_, &mock_verifier_);
    message_manager_ = std::make_unique<MessageManager>(
        config_, nullptr, checkpoint_manager_.get(), &system_info_);
    manager_ = std::make_unique<ViewChangeManager>(
        config_, checkpoint_manager_.get(), message_manager_.get(),
        &system_info_, &replica_communicator_, &mock_verifier_);

    ON_CALL(mock_verifier_, SignMessage).WillByDefault(Return(SignatureInfo()));
    ON_CALL(mock_verifier_, VerifyMessage(_, EqualsProto(SignatureInfo())))
        .WillByDefault(Return(true));
  }

 protected:
  ResDBConfig config_;
  MockSignatureVerifier mock_verifier_;
  SystemInfo system_info_;
  std::unique_ptr<CheckPointManager> checkpoint_manager_;
  std::unique_ptr<MessageManager> message_manager_;
  std::unique_ptr<ViewChangeManager> manager_;
  MockReplicaCommunicator replica_communicator_;
};

TEST_F(ViewChangeManagerTest, SendViewChange) {
  manager_->MayStart();
  std::unique_ptr<Request> request = std::make_unique<Request>();
  request->set_seq(1);
  checkpoint_manager_->AddCommitData(std::move(request));
  std::promise<bool> propose_done;
  std::future<bool> propose_done_future = propose_done.get_future();
  EXPECT_CALL(replica_communicator_, BroadCast)
      .WillOnce(Invoke([&](const google::protobuf::Message& message) {
        propose_done.set_value(true);
      }));
  propose_done_future.get();
}

TEST_F(ViewChangeManagerTest, SendNewView) {
  manager_->MayStart();
  std::promise<bool> propose_done;
  std::future<bool> propose_done_future = propose_done.get_future();
  EXPECT_CALL(replica_communicator_, BroadCast)
      .WillRepeatedly(Invoke([&](const google::protobuf::Message& message) {
        Request viewchange_request;
        viewchange_request.CopyFrom(message);
        if (viewchange_request.type() == Request::TYPE_NEWVIEW) {
          propose_done.set_value(true);
        }
      }));

  for (int i = 0; i < 3; ++i) {
    ViewChangeMessage viewchange_message;
    std::unique_ptr<Request> request =
        NewRequest(Request::TYPE_VIEWCHANGE, Request(), i + 1);
    viewchange_message.set_view_number(3);
    viewchange_message.SerializeToString(request->mutable_data());

    int ret = manager_->ProcessViewChange(std::make_unique<Context>(),
                                          std::move(request));
    EXPECT_EQ(ret, 0);
  }
  propose_done_future.get();
}

}  // namespace

}  // namespace resdb
