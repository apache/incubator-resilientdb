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

#include <unistd.h>

#include "glog/logging.h"

namespace resdb {

Query::Query(const ResDBConfig& config, TransactionManager* transaction_manager,
             std::unique_ptr<CustomQuery> executor)
    : config_(config),
      transaction_manager_(transaction_manager),
      custom_query_executor_(std::move(executor)) {}

Query::~Query() {}

int Query::ProcessGetReplicaState(std::unique_ptr<Context> context,
                                  std::unique_ptr<Request> request) {
  ReplicaState replica_state;
  int ret = transaction_manager_->GetReplicaState(&replica_state);
  if (ret == 0) {
    if (context != nullptr && context->client != nullptr) {
      ret = context->client->SendRawMessage(replica_state);
      if (ret) {
        LOG(ERROR) << "send resp" << replica_state.DebugString()
                   << " fail ret:" << ret;
      }
    }
  }
  return ret;
}

int Query::ProcessQuery(std::unique_ptr<Context> context,
                        std::unique_ptr<Request> request) {
  QueryRequest query;
  if (!query.ParseFromString(request->data())) {
    LOG(ERROR) << "parse data fail";
    return -2;
  }
  // LOG(ERROR) << "request:" << query.DebugString();

  QueryResponse response;

  for (uint64_t i = query.min_seq(); i <= query.max_seq(); ++i) {
    Request* ret_request = transaction_manager_->GetRequest(i);
    if (ret_request == nullptr) {
      break;
    }
    Request* txn = response.add_transactions();
    txn->set_data(ret_request->data());
    txn->set_seq(ret_request->seq());
  }

  if (context != nullptr && context->client != nullptr) {
    // LOG(ERROR) << "send response:" << response.DebugString();
    int ret = context->client->SendRawMessage(response);
    if (ret) {
      LOG(ERROR) << "send resp fail ret:" << ret;
    }
  }
  return 0;
}

int Query::ProcessCustomQuery(std::unique_ptr<Context> context,
                              std::unique_ptr<Request> request) {
  if (custom_query_executor_ == nullptr) {
    LOG(ERROR) << "no custom executor";
    return -1;
  }

  std::unique_ptr<std::string> resp_str =
      custom_query_executor_->Query(request->data());

  CustomQueryResponse response;
  if (resp_str != nullptr) {
    response.set_resp_str(*resp_str);
  }

  if (context != nullptr && context->client != nullptr) {
    int ret = context->client->SendRawMessage(response);
    if (ret) {
      LOG(ERROR) << "send resp fail ret:" << ret;
    }
  }
  return 0;
}

}  // namespace resdb
