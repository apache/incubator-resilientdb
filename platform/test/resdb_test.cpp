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

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <thread>

#include "executor/common/transaction_manager.h"
#include "interface/rdbc/transaction_constructor.h"
#include "platform/config/resdb_config_utils.h"
#include "platform/consensus/ordering/pbft/consensus_manager_pbft.h"
#include "platform/networkstrate/service_network.h"
#include "platform/test/proto/resdb_test.pb.h"

namespace resdb {

using ::testing::ElementsAreArray;
using ::testing::Test;

const std::string test_dir = std::string(getenv("TEST_SRCDIR")) + "/" +
                             std::string(getenv("TEST_WORKSPACE")) +
                             "/platform/test/";

class ResDBTestClient : public TransactionConstructor {
 public:
  ResDBTestClient(const ResDBConfig& config) : TransactionConstructor(config) {}

  int Set(int seq) {
    TestRequest request;
    request.set_seq(seq);
    TestResponse response;
    return SendRequest(request, &response);
  }
};

class ResDBTestManager : public TransactionManager {
 public:
  ResDBTestManager(const ResDBConfig& config) : config_(config) {}
  std::vector<int> GetSeqs() { return seqs_; }

 protected:
  std::unique_ptr<std::string> ExecuteData(const std::string& request) {
    TestRequest test_request;
    // TestResponse test_response;
    if (!test_request.ParseFromString(request)) {
      LOG(ERROR) << "parse data fail";
      return nullptr;
    }
    seqs_.push_back(test_request.seq());
    return nullptr;
  }

  ResDBConfig config_;
  std::vector<int> seqs_;
};

class ResDBTest : public Test {
 public:
  void StartServer() {}

  void StartOneServer(int server_id) {
    ResDBConfig config = GetConfig(server_id);
    config.SetTestMode(true);
    auto executor = std::make_unique<ResDBTestManager>(config);
    executors_.push_back(executor.get());
    server_.push_back(std::make_unique<ServiceNetwork>(
        config,
        std::make_unique<ConsensusManagerPBFT>(config, std::move(executor))));
    server_thread_.push_back(std::thread(
        [&](ServiceNetwork* server) { server->Run(); }, server_.back().get()));
  }

  void WaitAllServerStarted() {
    for (auto& server : server_) {
      while (true) {
        std::unique_lock<std::mutex> lk(mutex_);
        cv_.wait_for(lk, std::chrono::microseconds(1000),
                     [&] { return server->ServiceIsReady(); });
        if (server->ServiceIsReady()) {
          break;
        }
      }
    }
  }

  void WaitExecutorDone(int received_num) {
    for (auto executor : executors_) {
      while (static_cast<int>(executor->GetSeqs().size()) < received_num) {
        usleep(10000);
      }
    }
  }

  void Stop() {
    for (auto& server : server_) {
      server->Stop();
    }
    for (auto& th : server_thread_) {
      th.join();
    }
    server_.clear();
  }

  std::string GetPrivateKey(int id) {
    return test_dir + std::string("test_data/node") + std::to_string(id) +
           ".key.pri";
  }

  std::string GetCertFile(int id) {
    return test_dir + std::string("test_data/cert_") + std::to_string(id) +
           ".cert";
  }

  std::string GetConfiFile() { return test_dir + "test_data/server.config"; }

  ResDBConfig GetConfig(
      int id, std::optional<ReplicaInfo> replica_info = std::nullopt) {
    return *GenerateResDBConfig(GetConfiFile(), GetPrivateKey(id),
                                GetCertFile(id), replica_info);
  }

 protected:
  std::mutex mutex_;
  std::condition_variable cv_;
  std::vector<ResDBTestManager*> executors_;
  std::vector<std::unique_ptr<ServiceNetwork>> server_;
  std::vector<std::thread> server_thread_;
};

TEST_F(ResDBTest, TestPBFTService) {
  for (int i = 1; i <= 4; ++i) {
    StartOneServer(i);
  }
  WaitAllServerStarted();
  int client_id = 5;
  ReplicaInfo client_info = GenerateReplicaInfo(client_id, "127.0.0.1", 80000);
  auto config = GetConfig(client_id, client_info);
  ResDBTestClient client(config);
  EXPECT_EQ(client.Set(1), 0);

  WaitExecutorDone(1);

  for (auto executor : executors_) {
    EXPECT_THAT(executor->GetSeqs(), ElementsAreArray({1}));
  }

  Stop();
}

}  // namespace resdb
