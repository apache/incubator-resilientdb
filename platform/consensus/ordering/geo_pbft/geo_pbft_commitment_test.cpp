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

#include "platform/consensus/ordering/geo_pbft/geo_pbft_commitment.h"

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <future>

#include "platform/communication/mock_resdb_replica_client.h"
#include "platform/config/resdb_config_utils.h"
#include "platform/consensus/execution/mock_geo_global_executor.h"
#include "platform/consensus/execution/mock_transaction_executor_impl.h"
#include "platform/crypto/mock_signature_verifier.h"

namespace resdb {
namespace {
using ::testing::Invoke;
using ::testing::Return;
using ::testing::Test;

TEST(GeoCommitTest, GeoRequestFromOtherRegion) {
  ResConfigData config_data = ResConfigData();
  config_data.set_self_region_id(1);
  std::vector<ReplicaInfo> replicas = {
      GenerateReplicaInfo(1, "127.0.0.1", 1234),
      GenerateReplicaInfo(2, "127.0.0.1", 1235),
      GenerateReplicaInfo(3, "127.0.0.1", 1236),
      GenerateReplicaInfo(4, "127.0.0.1", 1237)};
  ResDBConfig config = ResDBConfig(
      replicas, GenerateReplicaInfo(1, "127.0.0.1", 1234), config_data);
  auto system_info = std::make_unique<SystemInfo>(config);
  MockResDBReplicaClient replica_client;
  std::unique_ptr<MockGeoGlobalExecutor> global_executor;
  std::unique_ptr<GeoPBFTCommitment> commitment;

  auto context = std::make_unique<Context>();
  context->signature.set_signature("signature");

  Request request;
  request.set_seq(1);
  request.mutable_region_info()->set_region_id(2);
  BatchClientRequest client_request;
  *client_request.mutable_committed_certs()->add_committed_certs() =
      SignatureInfo();
  client_request.SerializeToString(request.mutable_data());

  global_executor = std::make_unique<MockGeoGlobalExecutor>(
      std::make_unique<MockTransactionExecutorImpl>(), config);
  std::promise<bool> done;
  std::future<bool> done_future = done.get_future();
  EXPECT_CALL(replica_client, BroadCast).Times(1);
  EXPECT_CALL(*global_executor, Execute).Times(1).WillOnce(Invoke([&]() {
    done.set_value(true);
    return;
  }));

  MockSignatureVerifier verifier;
  EXPECT_CALL(verifier, VerifyMessage(::testing::_, ::testing::_))
      .WillOnce(Return(true));
  commitment = std::make_unique<GeoPBFTCommitment>(
      std::move(global_executor), config, std::move(system_info),
      &replica_client, &verifier);
  EXPECT_EQ(commitment->GeoProcessCcm(std::move(context),
                                      std::make_unique<Request>(request)),
            0);
  done_future.get();
}

TEST(GeoCommitTest, GeoRequestFromLocalRegion) {
  ResConfigData config_data = ResConfigData();
  config_data.set_self_region_id(1);
  std::vector<ReplicaInfo> replicas = {
      GenerateReplicaInfo(1, "127.0.0.1", 1234),
      GenerateReplicaInfo(2, "127.0.0.1", 1235),
      GenerateReplicaInfo(3, "127.0.0.1", 1236),
      GenerateReplicaInfo(4, "127.0.0.1", 1237)};
  ResDBConfig config = ResDBConfig(
      replicas, GenerateReplicaInfo(1, "127.0.0.1", 1234), config_data);
  auto system_info = std::make_unique<SystemInfo>(config);
  MockResDBReplicaClient replica_client;
  std::unique_ptr<MockGeoGlobalExecutor> global_executor;
  std::unique_ptr<GeoPBFTCommitment> commitment;

  auto context = std::make_unique<Context>();
  context->signature.set_signature("signature");
  Request request;
  request.set_seq(1);
  request.mutable_region_info()->set_region_id(1);
  global_executor = std::make_unique<MockGeoGlobalExecutor>(
      std::make_unique<MockTransactionExecutorImpl>(), config);

  BatchClientRequest client_request;
  *client_request.mutable_committed_certs()->add_committed_certs() =
      SignatureInfo();
  client_request.SerializeToString(request.mutable_data());
  EXPECT_CALL(replica_client, BroadCast).Times(0);

  std::promise<bool> done;
  std::future<bool> done_future = done.get_future();
  EXPECT_CALL(*global_executor, Execute).Times(1).WillOnce(Invoke([&]() {
    done.set_value(true);
    return;
  }));

  MockSignatureVerifier verifier;
  EXPECT_CALL(verifier, VerifyMessage(::testing::_, ::testing::_))
      .WillOnce(Return(true));
  commitment = std::make_unique<GeoPBFTCommitment>(
      std::move(global_executor), config, std::move(system_info),
      &replica_client, &verifier);
  EXPECT_EQ(commitment->GeoProcessCcm(std::move(context),
                                      std::make_unique<Request>(request)),
            0);
  done_future.get();
}

TEST(GeoCommitTest, ReceiveSameGeoRequest) {
  ResConfigData config_data = ResConfigData();
  config_data.set_self_region_id(1);
  std::vector<ReplicaInfo> replicas = {
      GenerateReplicaInfo(1, "127.0.0.1", 1234),
      GenerateReplicaInfo(2, "127.0.0.1", 1235),
      GenerateReplicaInfo(3, "127.0.0.1", 1236),
      GenerateReplicaInfo(4, "127.0.0.1", 1237)};
  ResDBConfig config = ResDBConfig(
      replicas, GenerateReplicaInfo(1, "127.0.0.1", 1234), config_data);
  auto system_info = std::make_unique<SystemInfo>(config);
  MockResDBReplicaClient replica_client;

  auto context = std::make_unique<Context>();
  context->signature.set_signature("signature");
  Request request;
  request.set_seq(1);
  request.mutable_region_info()->set_region_id(2);

  BatchClientRequest client_request;
  *client_request.mutable_committed_certs()->add_committed_certs() =
      SignatureInfo();
  client_request.SerializeToString(request.mutable_data());

  auto global_executor = std::make_unique<MockGeoGlobalExecutor>(
      std::make_unique<MockTransactionExecutorImpl>(), config);
  auto commitment = std::make_unique<GeoPBFTCommitment>(
      std::move(global_executor), config, std::move(system_info),
      &replica_client, nullptr);
  EXPECT_EQ(commitment->GeoProcessCcm(std::move(context),
                                      std::make_unique<Request>(request)),
            0);

  request.mutable_region_info()->set_region_id(1);
  auto context_2 = std::make_unique<Context>();
  context_2->signature.set_signature("signature");
  EXPECT_EQ(commitment->GeoProcessCcm(std::move(context_2),
                                      std::make_unique<Request>(request)),
            1);
  LOG(ERROR) << "done";
}

TEST(GeoCommitTest, NoCert) {
  ResConfigData config_data = ResConfigData();
  config_data.set_self_region_id(1);
  std::vector<ReplicaInfo> replicas = {
      GenerateReplicaInfo(1, "127.0.0.1", 1234),
      GenerateReplicaInfo(2, "127.0.0.1", 1235),
      GenerateReplicaInfo(3, "127.0.0.1", 1236),
      GenerateReplicaInfo(4, "127.0.0.1", 1237)};
  ResDBConfig config = ResDBConfig(
      replicas, GenerateReplicaInfo(1, "127.0.0.1", 1234), config_data);
  auto system_info = std::make_unique<SystemInfo>(config);
  MockResDBReplicaClient replica_client;
  std::unique_ptr<MockGeoGlobalExecutor> global_executor;
  std::unique_ptr<GeoPBFTCommitment> commitment;

  auto context = std::make_unique<Context>();
  context->signature.set_signature("signature");
  Request request;
  request.set_seq(1);
  request.mutable_region_info()->set_region_id(2);

  BatchClientRequest client_request;
  client_request.SerializeToString(request.mutable_data());

  MockSignatureVerifier verifier;
  EXPECT_CALL(verifier, VerifyMessage(::testing::_, ::testing::_)).Times(0);

  global_executor = std::make_unique<MockGeoGlobalExecutor>(
      std::make_unique<MockTransactionExecutorImpl>(), config);
  commitment = std::make_unique<GeoPBFTCommitment>(
      std::move(global_executor), config, std::move(system_info),
      &replica_client, &verifier);
  EXPECT_EQ(commitment->GeoProcessCcm(std::move(context),
                                      std::make_unique<Request>(request)),
            -2);
}

}  // namespace
}  // namespace resdb
