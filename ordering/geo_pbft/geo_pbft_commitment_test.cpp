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

#include "geo_pbft_commitment.h"

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <future>

#include "config/resdb_config_utils.h"
#include "crypto/mock_signature_verifier.h"
#include "execution/mock_geo_global_executor.h"
#include "execution/mock_transaction_executor_impl.h"
#include "server/mock_resdb_replica_client.h"

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
