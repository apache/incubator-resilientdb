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

#include "ordering/pbft/query.h"

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <future>

#include "client/mock_resdb_client.h"
#include "common/test/test_macros.h"
#include "config/resdb_config_utils.h"
#include "crypto/mock_signature_verifier.h"
#include "execution/mock_custom_query.h"
#include "ordering/pbft/commitment.h"
#include "ordering/pbft/transaction_manager.h"
#include "server/mock_resdb_replica_client.h"

namespace resdb {
namespace {

using ::resdb::testing::EqualsProto;
using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::Test;

class QueryTest : public Test {
 public:
  QueryTest()
      : global_stats_(Stats::GetGlobalStats(1)),
        config_({GenerateReplicaInfo(1, "127.0.0.1", 1234),
                 GenerateReplicaInfo(2, "127.0.0.1", 1235),
                 GenerateReplicaInfo(3, "127.0.0.1", 1236),
                 GenerateReplicaInfo(4, "127.0.0.1", 1237)},
                GenerateReplicaInfo(1, "127.0.0.1", 1234)),
        system_info_(config_),
        checkpoint_manager_(config_, &replica_client_, nullptr),
        transaction_manager_(config_, nullptr, &checkpoint_manager_,
                             &system_info_),
        query_(config_, &transaction_manager_),
        commitment_(config_, &transaction_manager_, &replica_client_,
                    &verifier_) {}

  int AddProposeMsg(int sender_id, bool need_resp = false, int proxy_id = 1) {
    auto context = std::make_unique<Context>();
    context->signature.set_signature("signature");

    Request request;
    request.set_current_view(1);
    request.set_seq(1);
    request.set_type(Request::TYPE_PRE_PREPARE);
    request.set_sender_id(sender_id);
    request.set_need_response(need_resp);
    request.set_proxy_id(proxy_id);

    return commitment_.ProcessProposeMsg(std::move(context),
                                         std::make_unique<Request>(request));
  }

  int AddPrepareMsg(int sender_id) {
    auto context = std::make_unique<Context>();
    context->signature.set_signature("signature");

    Request request;
    request.set_current_view(1);
    request.set_seq(1);
    request.set_type(Request::TYPE_PREPARE);
    request.set_sender_id(sender_id);
    return commitment_.ProcessPrepareMsg(std::move(context),
                                         std::make_unique<Request>(request));
  }

  int AddCommitMsg(int sender_id) {
    auto context = std::make_unique<Context>();
    context->signature.set_signature("signature");

    Request request;
    request.set_current_view(1);
    request.set_seq(1);
    request.set_type(Request::TYPE_COMMIT);
    request.set_sender_id(sender_id);
    return commitment_.ProcessCommitMsg(std::move(context),
                                        std::make_unique<Request>(request));
  }

  void CommitMsg() {
    std::promise<bool> done;
    std::future<bool> done_future = done.get_future();

    EXPECT_CALL(replica_client_, SendMessage(_, 1))
        .WillOnce(Invoke(
            [&](const google::protobuf::Message& request, int64_t node_id) {
              done.set_value(true);
              return 0;
            }));

    EXPECT_EQ(AddProposeMsg(1, true), 0);

    EXPECT_EQ(AddPrepareMsg(1), 0);
    EXPECT_EQ(AddPrepareMsg(2), 0);
    EXPECT_EQ(AddPrepareMsg(3), 0);

    EXPECT_EQ(AddCommitMsg(1), 0);
    EXPECT_EQ(AddCommitMsg(2), 0);
    EXPECT_EQ(AddCommitMsg(3), 0);
    done_future.get();
  }

 protected:
  Stats* global_stats_;
  ResDBConfig config_;
  SystemInfo system_info_;
  CheckPointManager checkpoint_manager_;
  TransactionManager transaction_manager_;
  Query query_;
  MockResDBReplicaClient replica_client_;
  MockSignatureVerifier verifier_;
  Commitment commitment_;
};

TEST_F(QueryTest, QueryState) {
  ReplicaState replica_state;
  replica_state.set_view(1);
  replica_state.mutable_replica_info()->set_id(1);
  replica_state.mutable_replica_info()->set_ip("127.0.0.1");
  replica_state.mutable_replica_info()->set_port(1234);

  std::unique_ptr<MockResDBClient> resp_client =
      std::make_unique<MockResDBClient>("127.0.0.1", 0);
  EXPECT_CALL(*resp_client, SendRawMessage(EqualsProto(replica_state)))
      .Times(1);

  auto context = std::make_unique<Context>();
  context->client = std::move(resp_client);
  context->signature.set_signature("signature");

  int ret = query_.ProcessGetReplicaState(std::move(context), nullptr);
  EXPECT_EQ(ret, 0);
}

TEST_F(QueryTest, QueryTxn) {
  CommitMsg();

  QueryResponse response;
  auto txn = response.add_transactions();
  txn->set_seq(1);

  std::unique_ptr<MockResDBClient> resp_client =
      std::make_unique<MockResDBClient>("127.0.0.1", 0);
  EXPECT_CALL(*resp_client, SendRawMessage(EqualsProto(response))).Times(1);

  auto context = std::make_unique<Context>();
  context->client = std::move(resp_client);

  context->signature.set_signature("signature");

  Request request;
  QueryRequest query;
  query.set_min_seq(1);
  query.set_max_seq(2);
  query.SerializeToString(request.mutable_data());

  int ret = query_.ProcessQuery(std::move(context),
                                std::make_unique<Request>(request));
  EXPECT_EQ(ret, 0);
}

TEST_F(QueryTest, CustomQuery) {
  CustomQueryResponse response;
  response.set_resp_str("custom_response");

  std::unique_ptr<MockResDBClient> resp_client =
      std::make_unique<MockResDBClient>("127.0.0.1", 0);
  EXPECT_CALL(*resp_client, SendRawMessage(EqualsProto(response))).Times(1);

  auto context = std::make_unique<Context>();
  context->client = std::move(resp_client);

  context->signature.set_signature("signature");

  Request request;
  request.set_data("request");

  auto custom_query_executor = std::make_unique<MockCustomQuery>();
  EXPECT_CALL(*custom_query_executor, Query("request"))
      .WillOnce(Invoke([&](const std::string& str) {
        return std::make_unique<std::string>("custom_response");
      }));

  Query cus_query(config_, nullptr, std::move(custom_query_executor));

  int ret = cus_query.ProcessCustomQuery(std::move(context),
                                         std::make_unique<Request>(request));
  EXPECT_EQ(ret, 0);
}

}  // namespace

}  // namespace resdb
