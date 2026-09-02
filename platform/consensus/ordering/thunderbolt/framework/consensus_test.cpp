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
#include "platform/consensus/ordering/cassandra/framework/consensus.h"

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <future>

#include "common/test/test_macros.h"
#include "executor/common/mock_transaction_manager.h"
#include "platform/config/resdb_config_utils.h"
#include "platform/networkstrate/mock_replica_communicator.h"

namespace resdb {
namespace cassandra {
namespace {

using ::resdb::testing::EqualsProto;
using ::testing::_;
using ::testing::Invoke;
using ::testing::Test;

ResDBConfig GetConfig() {
  ResDBConfig config({GenerateReplicaInfo(1, "127.0.0.1", 1234),
                      GenerateReplicaInfo(2, "127.0.0.1", 1235),
                      GenerateReplicaInfo(3, "127.0.0.1", 1236),
                      GenerateReplicaInfo(4, "127.0.0.1", 1237)},
                     GenerateReplicaInfo(1, "127.0.0.1", 1234));
  return config;
}

class ConsensusTest : public Test {
 public:
  ConsensusTest() : config_(GetConfig()) {
    auto transaction_manager =
        std::make_unique<MockTransactionExecutorDataImpl>();
    mock_transaction_manager_ = transaction_manager.get();
    consensus_ =
        std::make_unique<Consensus>(config_, std::move(transaction_manager));
    consensus_->SetCommunicator(&replica_communicator_);
  }

  void AddTransaction(const std::string& data) {
    auto request = std::make_unique<Request>();
    request->set_type(Request::TYPE_NEW_TXNS);

    Transaction txn;

    BatchUserRequest batch_request;
    auto req = batch_request.add_user_requests();
    req->mutable_request()->set_data(data);

    batch_request.set_local_id(1);
    batch_request.SerializeToString(txn.mutable_data());

    txn.SerializeToString(request->mutable_data());

    EXPECT_EQ(consensus_->ConsensusCommit(nullptr, std::move(request)), 0);
  }

 protected:
  ResDBConfig config_;
  MockTransactionExecutorDataImpl* mock_transaction_manager_;
  MockReplicaCommunicator replica_communicator_;
  std::unique_ptr<TransactionManager> transaction_manager_;
  std::unique_ptr<Consensus> consensus_;
};

TEST_F(ConsensusTest, NormalCase) {
  std::promise<bool> commit_done;
  std::future<bool> commit_done_future = commit_done.get_future();

  EXPECT_CALL(replica_communicator_, BroadCast)
      .WillRepeatedly(Invoke([&](const google::protobuf::Message& msg) {
        Request request = *dynamic_cast<const Request*>(&msg);

        if (request.user_type() == MessageType::NewProposal) {
          LOG(ERROR) << "bc new proposal";
          consensus_->ConsensusCommit(nullptr,
                                      std::make_unique<Request>(request));
          LOG(ERROR) << "recv proposal done";
        }
        if (request.user_type() == MessageType::Vote) {
          LOG(ERROR) << "bc vote";

          VoteMessage ack_msg;
          assert(ack_msg.ParseFromString(request.data()));
          for (int i = 1; i <= 3; ++i) {
            ack_msg.set_proposer_id(i);
            auto new_req = std::make_unique<Request>(request);
            ack_msg.SerializeToString(new_req->mutable_data());

            consensus_->ConsensusCommit(nullptr, std::move(new_req));
          }
        }
        // LOG(ERROR)<<"bc type:"<<request->type()<<" user
        // type:"<<request->user_type();
        if (request.user_type() == MessageType::Prepare) {
          LOG(ERROR) << "bc prepare";

          VoteMessage ack_msg;
          assert(ack_msg.ParseFromString(request.data()));
          for (int i = 1; i <= 3; ++i) {
            ack_msg.set_proposer_id(i);
            auto new_req = std::make_unique<Request>(request);
            ack_msg.SerializeToString(new_req->mutable_data());

            consensus_->ConsensusCommit(nullptr, std::move(new_req));
          }
        }
        if (request.user_type() == MessageType::Voteprep) {
          LOG(ERROR) << "bc voterep:";

          VoteMessage ack_msg;
          assert(ack_msg.ParseFromString(request.data()));
          for (int i = 1; i <= 3; ++i) {
            ack_msg.set_proposer_id(i);
            auto new_req = std::make_unique<Request>(request);
            ack_msg.SerializeToString(new_req->mutable_data());
            LOG(ERROR) << "new request type:" << new_req->user_type();

            consensus_->ConsensusCommit(nullptr, std::move(new_req));
          }
        }
        LOG(ERROR) << "done";
        return 0;
      }));

  EXPECT_CALL(*mock_transaction_manager_, ExecuteData)
      .WillOnce(Invoke([&](const std::string& msg) {
        LOG(ERROR) << "execute txn:" << msg;
        EXPECT_EQ(msg, "transaction1");
        return nullptr;
      }));

  EXPECT_CALL(replica_communicator_, SendMessage(_, 0))
      .WillRepeatedly(
          Invoke([&](const google::protobuf::Message& msg, int64_t) {
            Request request = *dynamic_cast<const Request*>(&msg);
            if (request.type() == Request::TYPE_RESPONSE) {
              LOG(ERROR) << "get response";
              commit_done.set_value(true);
            }
            return;
          }));

  AddTransaction("transaction1");

  commit_done_future.get();
}

}  // namespace
}  // namespace cassandra
}  // namespace resdb
