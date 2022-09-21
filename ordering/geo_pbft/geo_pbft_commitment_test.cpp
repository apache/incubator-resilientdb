#include "geo_pbft_commitment.h"

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <future>

#include "config/resdb_config_utils.h"
#include "execution/mock_geo_global_executor.h"
#include "execution/mock_transaction_executor_impl.h"
#include "server/mock_resdb_replica_client.h"

namespace resdb {
namespace {
using ::testing::Return;
using ::testing::Test;

TEST(GeoCommitTest, GeoRequestFromOtherRegion) {
  ResConfigData config_data_ = ResConfigData();
  config_data_.set_self_region_id(1);
  std::vector<ReplicaInfo> replicas = {
      GenerateReplicaInfo(1, "127.0.0.1", 1234),
      GenerateReplicaInfo(2, "127.0.0.1", 1235),
      GenerateReplicaInfo(3, "127.0.0.1", 1236),
      GenerateReplicaInfo(4, "127.0.0.1", 1237)};
  ResDBConfig config_ = ResDBConfig(
      replicas, GenerateReplicaInfo(1, "127.0.0.1", 1234), config_data_);
  auto system_info_ = std::make_unique<SystemInfo>(config_);
  MockResDBReplicaClient replica_client_;
  std::unique_ptr<MockGeoGlobalExecutor> global_executor_;
  std::unique_ptr<GeoPBFTCommitment> commitment_;

  auto context = std::make_unique<Context>();
  context->signature.set_signature("signature");

  Request request;
  request.set_seq(1);
  request.mutable_region_info()->set_region_id(2);
  global_executor_ = std::make_unique<MockGeoGlobalExecutor>(
      std::make_unique<MockTransactionExecutorImpl>());
  EXPECT_CALL(replica_client_, BroadCast).Times(1);
  EXPECT_CALL(*global_executor_, Execute).Times(1).WillRepeatedly(Return(0));

  commitment_ = std::make_unique<GeoPBFTCommitment>(
      std::move(global_executor_), config_, std::move(system_info_),
      &replica_client_);
  EXPECT_EQ(commitment_->GeoProcessCcm(std::move(context),
                                       std::make_unique<Request>(request)),
            0);
}

TEST(GeoCommitTest, GeoRequestFromLocalRegion) {
  ResConfigData config_data_ = ResConfigData();
  config_data_.set_self_region_id(1);
  std::vector<ReplicaInfo> replicas = {
      GenerateReplicaInfo(1, "127.0.0.1", 1234),
      GenerateReplicaInfo(2, "127.0.0.1", 1235),
      GenerateReplicaInfo(3, "127.0.0.1", 1236),
      GenerateReplicaInfo(4, "127.0.0.1", 1237)};
  ResDBConfig config_ = ResDBConfig(
      replicas, GenerateReplicaInfo(1, "127.0.0.1", 1234), config_data_);
  auto system_info_ = std::make_unique<SystemInfo>(config_);
  MockResDBReplicaClient replica_client_;
  std::unique_ptr<MockGeoGlobalExecutor> global_executor_;
  std::unique_ptr<GeoPBFTCommitment> commitment_;

  auto context = std::make_unique<Context>();
  context->signature.set_signature("signature");
  Request request;
  request.set_seq(1);
  request.mutable_region_info()->set_region_id(1);
  global_executor_ = std::make_unique<MockGeoGlobalExecutor>(
      std::make_unique<MockTransactionExecutorImpl>());
  EXPECT_CALL(replica_client_, BroadCast).Times(0);
  EXPECT_CALL(*global_executor_, Execute).Times(1).WillRepeatedly(Return(0));
  commitment_ = std::make_unique<GeoPBFTCommitment>(
      std::move(global_executor_), config_, std::move(system_info_),
      &replica_client_);
  EXPECT_EQ(commitment_->GeoProcessCcm(std::move(context),
                                       std::make_unique<Request>(request)),
            0);
}

TEST(GeoCommitTest, ReceiveSameGeoRequest) {
  ResConfigData config_data_ = ResConfigData();
  config_data_.set_self_region_id(1);
  std::vector<ReplicaInfo> replicas = {
      GenerateReplicaInfo(1, "127.0.0.1", 1234),
      GenerateReplicaInfo(2, "127.0.0.1", 1235),
      GenerateReplicaInfo(3, "127.0.0.1", 1236),
      GenerateReplicaInfo(4, "127.0.0.1", 1237)};
  ResDBConfig config_ = ResDBConfig(
      replicas, GenerateReplicaInfo(1, "127.0.0.1", 1234), config_data_);
  auto system_info_ = std::make_unique<SystemInfo>(config_);
  MockResDBReplicaClient replica_client_;
  std::unique_ptr<MockGeoGlobalExecutor> global_executor_;
  std::unique_ptr<GeoPBFTCommitment> commitment_;

  auto context = std::make_unique<Context>();
  context->signature.set_signature("signature");
  Request request;
  request.set_seq(1);
  request.mutable_region_info()->set_region_id(2);
  global_executor_ = std::make_unique<MockGeoGlobalExecutor>(
      std::make_unique<MockTransactionExecutorImpl>());
  EXPECT_CALL(replica_client_, BroadCast).Times(1);
  EXPECT_CALL(*global_executor_, Execute).Times(1).WillRepeatedly(Return(0));
  commitment_ = std::make_unique<GeoPBFTCommitment>(
      std::move(global_executor_), config_, std::move(system_info_),
      &replica_client_);
  EXPECT_EQ(commitment_->GeoProcessCcm(std::move(context),
                                       std::make_unique<Request>(request)),
            0);

  request.mutable_region_info()->set_region_id(1);
  auto context_2 = std::make_unique<Context>();
  context_2->signature.set_signature("signature");
  EXPECT_EQ(commitment_->GeoProcessCcm(std::move(context_2),
                                       std::make_unique<Request>(request)),
            1);
}

}  // namespace
}  // namespace resdb
