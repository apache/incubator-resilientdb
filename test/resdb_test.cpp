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

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <thread>

#include "client/resdb_user_client.h"
#include "config/resdb_config_utils.h"
#include "execution/transaction_executor_impl.h"
#include "ordering/pbft/consensus_service_pbft.h"
#include "server/resdb_server.h"
#include "test/proto/resdb_test.pb.h"

namespace resdb {

using ::testing::ElementsAreArray;
using ::testing::Test;

const std::string test_dir = std::string(getenv("TEST_SRCDIR")) + "/" +
                             std::string(getenv("TEST_WORKSPACE")) + "/test/";

class ResDBTestClient : public ResDBUserClient {
 public:
  ResDBTestClient(const ResDBConfig& config) : ResDBUserClient(config) {}

  int Set(int seq) {
    TestRequest request;
    request.set_seq(seq);
    TestResponse response;
    return SendRequest(request, &response);
  }
};

class ResDBTestExecutor : public TransactionExecutorImpl {
 public:
  ResDBTestExecutor(const ResDBConfig& config) : config_(config) {}
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
    auto executor = std::make_unique<ResDBTestExecutor>(config);
    executors_.push_back(executor.get());
    server_.push_back(std::make_unique<ResDBServer>(
        config,
        std::make_unique<ConsensusServicePBFT>(config, std::move(executor))));
    server_thread_.push_back(std::thread(
        [&](ResDBServer* server) { server->Run(); }, server_.back().get()));
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
      while (executor->GetSeqs().size() < received_num) {
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
  std::vector<ResDBTestExecutor*> executors_;
  std::vector<std::unique_ptr<ResDBServer>> server_;
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
