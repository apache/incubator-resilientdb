#include "geo_transaction_executor.h"

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <future>

#include "common/test/test_macros.h"
#include "config/resdb_config_utils.h"
#include "mock_transaction_executor_impl.h"
#include "ordering/pbft/transaction_utils.h"
#include "server/mock_resdb_replica_client.h"

namespace resdb {
namespace {
using ::resdb::testing::EqualsProto;
using ::testing::_;
using ::testing::Invoke;
using ::testing::Matcher;
using ::testing::Return;
using ::testing::Test;

MATCHER_P(SendMessageMatcher, replica, "") { return true; }

TEST(LocalExecutorTest, NonPrimaryNode) {
  auto mock_executor = std::make_unique<MockTransactionExecutorImpl>();
  ResConfigData config_data_ = ResConfigData();
  config_data_.set_self_region_id(1);

  // setup region id=1
  RegionInfo* region = config_data_.add_region();
  region->set_region_id(1);
  ReplicaInfo* replica = region->add_replica_info();
  *replica = GenerateReplicaInfo(1, "127.0.0.1", 10001);
  replica = region->add_replica_info();
  *replica = GenerateReplicaInfo(2, "127.0.0.1", 10002);
  replica = region->add_replica_info();
  *replica = GenerateReplicaInfo(3, "127.0.0.1", 10003);
  replica = region->add_replica_info();
  *replica = GenerateReplicaInfo(4, "127.0.0.1", 10004);

  // setup region id=2
  region = config_data_.add_region();
  region->set_region_id(2);
  *region->add_replica_info() = GenerateReplicaInfo(6, "127.0.0.1", 10006);
  *region->add_replica_info() = GenerateReplicaInfo(7, "127.0.0.1", 10007);
  *region->add_replica_info() = GenerateReplicaInfo(8, "127.0.0.1", 10008);
  *region->add_replica_info() = GenerateReplicaInfo(9, "127.0.0.1", 10009);

  ResDBConfig config_ =
      ResDBConfig(config_data_, GenerateReplicaInfo(1, "127.0.0.1", 10001),
                  KeyInfo(), CertificateInfo());
  auto system_info_ = std::make_unique<SystemInfo>(config_);
  // primary node id: 2
  system_info_->SetPrimary(2);
  Request request;
  request.set_seq(1);
  BatchClientRequest batch_request;
  batch_request.add_client_requests()->mutable_request()->set_data(
      "local_execute_test");
  batch_request.SerializeToString(request.mutable_data());

  EXPECT_CALL(*mock_executor, ExecuteBatch)
      .Times(1)
      .WillRepeatedly(Invoke([&](const BatchClientRequest& br) {
        return std::make_unique<BatchClientResponse>();
      }));

  GeoTransactionExecutor local_executor = GeoTransactionExecutor(
      config_, std::move(system_info_),
      std::make_unique<MockResDBReplicaClient>(), std::move(mock_executor));
  EXPECT_THAT(*(local_executor.ExecuteBatch(batch_request)),
              EqualsProto(BatchClientResponse()));
}

TEST(LocalExecutorTest, PrimaryNodeBroadcast) {
  auto mock_executor = std::make_unique<MockTransactionExecutorImpl>();
  auto replica_client = std::make_unique<MockResDBReplicaClient>();
  ResConfigData config_data_ = ResConfigData();
  config_data_.set_self_region_id(1);

  // setup region id=1
  RegionInfo* region = config_data_.add_region();
  region->set_region_id(1);
  ReplicaInfo* replica = region->add_replica_info();
  *replica = GenerateReplicaInfo(1, "127.0.0.1", 10001);
  replica = region->add_replica_info();
  *replica = GenerateReplicaInfo(2, "127.0.0.1", 10002);
  replica = region->add_replica_info();
  *replica = GenerateReplicaInfo(3, "127.0.0.1", 10003);
  replica = region->add_replica_info();
  *replica = GenerateReplicaInfo(4, "127.0.0.1", 10004);

  // setup region id=2
  region = config_data_.add_region();
  region->set_region_id(2);
  *region->add_replica_info() = GenerateReplicaInfo(6, "127.0.0.1", 10006);
  *region->add_replica_info() = GenerateReplicaInfo(7, "127.0.0.1", 10007);
  *region->add_replica_info() = GenerateReplicaInfo(8, "127.0.0.1", 10008);
  *region->add_replica_info() = GenerateReplicaInfo(9, "127.0.0.1", 10009);

  ResDBConfig config_ =
      ResDBConfig(config_data_, GenerateReplicaInfo(1, "127.0.0.1", 10001),
                  KeyInfo(), CertificateInfo());
  auto system_info_ = std::make_unique<SystemInfo>(config_);
  // primary node id: 1
  system_info_->SetPrimary(1);
  Request request;
  request.set_seq(1);
  BatchClientRequest batch_request;
  batch_request.add_client_requests()->mutable_request()->set_data(
      "local_execute_test");
  batch_request.SerializeToString(request.mutable_data());
  std::unique_ptr<Request> geo_request = resdb::NewRequest(
      Request::TYPE_GEO_REQUEST, Request(), config_.GetSelfInfo().id(),
      config_data_.self_region_id());

  ReplicaInfo rep_info = GenerateReplicaInfo(1, "127.0.0.1", 10001);
  request.SerializeToString(geo_request->mutable_data());

  EXPECT_CALL(
      *replica_client,
      SendMessage(Matcher<const google::protobuf::Message&>(
                      SendMessageMatcher(*geo_request)),
                  Matcher<const ReplicaInfo&>(SendMessageMatcher(rep_info))))
      .WillRepeatedly(Return(0));

  EXPECT_CALL(*mock_executor, ExecuteBatch)
      .Times(1)
      .WillRepeatedly(Invoke([&](const BatchClientRequest& br) {
        return std::make_unique<BatchClientResponse>();
      }));

  GeoTransactionExecutor local_executor = GeoTransactionExecutor(
      config_, std::move(system_info_), std::move(replica_client),
      std::move(mock_executor));
  EXPECT_THAT(*(local_executor.ExecuteBatch(batch_request)),
              EqualsProto(BatchClientResponse()));
}

}  // namespace

}  // namespace resdb
