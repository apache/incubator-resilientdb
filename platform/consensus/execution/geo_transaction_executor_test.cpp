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

#include "platform/consensus/execution/geo_transaction_executor.h"

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <future>

#include "common/test/test_macros.h"
#include "executor/common/mock_transaction_manager.h"
#include "platform/config/resdb_config_utils.h"
#include "platform/consensus/ordering/common/transaction_utils.h"
#include "platform/networkstrate/mock_replica_communicator.h"

namespace resdb {
namespace {
using ::resdb::testing::EqualsProto;
using ::testing::_;
using ::testing::Invoke;
using ::testing::Matcher;
using ::testing::Return;
using ::testing::Test;

MATCHER_P(SendMessageMatcher, replica, "") { return true; }

TEST(LocalExecutorTest, PushToQueue) {
  auto mock_executor = std::make_unique<MockTransactionManager>();
  auto replica_communicator = std::make_unique<MockReplicaCommunicator>();
  ResConfigData config_data_ = ResConfigData();
  config_data_.set_self_region_id(1);

  // setup region id=1
  RegionInfo* region = config_data_.add_region();
  region->set_region_id(1);
  *region->add_replica_info() = GenerateReplicaInfo(1, "127.0.0.1", 10001);
  *region->add_replica_info() = GenerateReplicaInfo(2, "127.0.0.1", 10002);
  *region->add_replica_info() = GenerateReplicaInfo(3, "127.0.0.1", 10003);
  *region->add_replica_info() = GenerateReplicaInfo(4, "127.0.0.1", 10004);

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
  BatchUserRequest batch_request;
  batch_request.set_seq(1);
  batch_request.add_user_requests()->mutable_request()->set_data(
      "local_execute_test");

  ReplicaInfo rep_info = GenerateReplicaInfo(1, "127.0.0.1", 10001);

  std::promise<bool> done;
  std::future<bool> done_future = done.get_future();
  EXPECT_CALL(*replica_communicator, SendBatchMessage).WillOnce(Invoke([&]() {
    done.set_value(true);
    return 0;
  }));

  GeoTransactionExecutor local_executor = GeoTransactionExecutor(
      config_, std::move(system_info_), std::move(replica_communicator),
      std::move(mock_executor));
  LOG(ERROR) << "exe";
  local_executor.ExecuteBatch(batch_request);
  done_future.get();
}

TEST(LocalExecutorTest, PrimaryNodeBroadcast) {
  auto mock_executor = std::make_unique<MockTransactionManager>();
  auto replica_communicator = std::make_unique<MockReplicaCommunicator>();
  ResConfigData config_data_ = ResConfigData();
  config_data_.set_self_region_id(1);

  // setup region id=1
  RegionInfo* region = config_data_.add_region();
  region->set_region_id(1);
  *region->add_replica_info() = GenerateReplicaInfo(1, "127.0.0.1", 10001);
  *region->add_replica_info() = GenerateReplicaInfo(2, "127.0.0.1", 10002);
  *region->add_replica_info() = GenerateReplicaInfo(3, "127.0.0.1", 10003);
  *region->add_replica_info() = GenerateReplicaInfo(4, "127.0.0.1", 10004);

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
  BatchUserRequest batch_request;
  batch_request.set_seq(1);
  batch_request.add_user_requests()->mutable_request()->set_data(
      "local_execute_test");

  ReplicaInfo rep_info = GenerateReplicaInfo(1, "127.0.0.1", 10001);

  int call_times = 0;
  std::promise<bool> done;
  std::future<bool> done_future = done.get_future();
  EXPECT_CALL(*replica_communicator, SendBatchMessage)
      .WillRepeatedly(Invoke([&]() {
        if (++call_times >= 3) {
          done.set_value(true);
        }
        return 0;
      }));

  GeoTransactionExecutor local_executor = GeoTransactionExecutor(
      config_, std::move(system_info_), std::move(replica_communicator),
      std::move(mock_executor));
  local_executor.ExecuteBatch(batch_request);
  done_future.get();
}

}  // namespace

}  // namespace resdb
