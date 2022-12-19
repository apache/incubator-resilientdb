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

TEST(LocalExecutorTest, PushToQueue) {
  auto mock_executor = std::make_unique<MockTransactionExecutorImpl>();
  auto replica_client = std::make_unique<MockResDBReplicaClient>();
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
  BatchClientRequest batch_request;
  batch_request.set_seq(1);
  batch_request.add_client_requests()->mutable_request()->set_data(
      "local_execute_test");

  ReplicaInfo rep_info = GenerateReplicaInfo(1, "127.0.0.1", 10001);

  std::promise<bool> done;
  std::future<bool> done_future = done.get_future();
  EXPECT_CALL(*replica_client, SendBatchMessage).WillOnce(Invoke([&]() {
    done.set_value(true);
    return 0;
  }));

  GeoTransactionExecutor local_executor = GeoTransactionExecutor(
      config_, std::move(system_info_), std::move(replica_client),
      std::move(mock_executor));
  LOG(ERROR) << "exe";
  local_executor.ExecuteBatch(batch_request);
  done_future.get();
}

TEST(LocalExecutorTest, PrimaryNodeBroadcast) {
  auto mock_executor = std::make_unique<MockTransactionExecutorImpl>();
  auto replica_client = std::make_unique<MockResDBReplicaClient>();
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
  BatchClientRequest batch_request;
  batch_request.set_seq(1);
  batch_request.add_client_requests()->mutable_request()->set_data(
      "local_execute_test");

  ReplicaInfo rep_info = GenerateReplicaInfo(1, "127.0.0.1", 10001);

  int call_times = 0;
  std::promise<bool> done;
  std::future<bool> done_future = done.get_future();
  EXPECT_CALL(*replica_client, SendBatchMessage).WillRepeatedly(Invoke([&]() {
    if (++call_times >= 3) {
      done.set_value(true);
    }
    return 0;
  }));

  GeoTransactionExecutor local_executor = GeoTransactionExecutor(
      config_, std::move(system_info_), std::move(replica_client),
      std::move(mock_executor));
  local_executor.ExecuteBatch(batch_request);
  done_future.get();
}

}  // namespace

}  // namespace resdb
