#include "ordering/pbft/viewchange_manager.h"

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <future>

#include "common/test/test_macros.h"
#include "config/resdb_config_utils.h"
#include "crypto/mock_signature_verifier.h"
#include "execution/system_info.h"
#include "ordering/pbft/checkpoint_manager.h"
#include "ordering/pbft/transaction_utils.h"
#include "proto/checkpoint_info.pb.h"
#include "server/mock_resdb_replica_client.h"

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
        config_, &replica_client_, &mock_verifier_);
    transaction_manager_ = std::make_unique<TransactionManager>(
        config_, nullptr, checkpoint_manager_.get(), &system_info_);
    manager_ = std::make_unique<ViewChangeManager>(
        config_, checkpoint_manager_.get(), transaction_manager_.get(),
        &system_info_, &replica_client_, &mock_verifier_);

    ON_CALL(mock_verifier_, SignMessage).WillByDefault(Return(SignatureInfo()));
    ON_CALL(mock_verifier_, VerifyMessage(_, EqualsProto(SignatureInfo())))
        .WillByDefault(Return(true));
  }

 protected:
  ResDBConfig config_;
  MockSignatureVerifier mock_verifier_;
  SystemInfo system_info_;
  std::unique_ptr<CheckPointManager> checkpoint_manager_;
  std::unique_ptr<TransactionManager> transaction_manager_;
  std::unique_ptr<ViewChangeManager> manager_;
  MockResDBReplicaClient replica_client_;
};

TEST_F(ViewChangeManagerTest, SendViewChange) {
  manager_->MayStart();
  std::promise<bool> propose_done;
  std::future<bool> propose_done_future = propose_done.get_future();
  EXPECT_CALL(replica_client_, BroadCast)
      .WillOnce(Invoke([&](const google::protobuf::Message& message) {
        LOG(ERROR) << "bc:";
        propose_done.set_value(true);
      }));
  propose_done_future.get();
}

TEST_F(ViewChangeManagerTest, SendNewView) {
  manager_->MayStart();
  std::promise<bool> propose_done;
  std::future<bool> propose_done_future = propose_done.get_future();
  EXPECT_CALL(replica_client_, BroadCast)
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
