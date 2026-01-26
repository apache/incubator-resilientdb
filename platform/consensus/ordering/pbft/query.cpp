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

#include "platform/consensus/ordering/pbft/query.h"

#include <glog/logging.h>
#include <unistd.h>

namespace resdb {

Query::Query(const ResDBConfig& config, Recovery* recovery,
             std::unique_ptr<CustomQuery> executor)
    : config_(config),
      recovery_(recovery),
      custom_query_executor_(std::move(executor)) {}

Query::~Query() {}

int Query::ProcessGetReplicaState(std::unique_ptr<Context> context,
                                  std::unique_ptr<Request> request) {
  ReplicaState replica_state;

  *replica_state.mutable_replica_config() = config_.GetConfigData();

  if (context != nullptr && context->client != nullptr) {
    int ret = context->client->SendRawMessage(replica_state);
    if (ret) {
      LOG(ERROR) << "send resp" << replica_state.DebugString()
                 << " fail ret:" << ret;
    }
  }
  return 0;
}

int Query::ProcessQuery(std::unique_ptr<Context> context,
                        std::unique_ptr<Request> request) {
  if (config_.GetPublicKeyCertificateInfo()
          .public_key()
          .public_key_info()
          .type() == CertificateKeyInfo::CLIENT) {
    auto find_primary = [&]() {
      auto config_data = config_.GetConfigData();
      for (const auto& r : config_data.region()) {
        for (const auto& replica : r.replica_info()) {
          if (replica.id() == 1) {
            return replica;
          }
        }
      }
    };

    ReplicaInfo primary = find_primary();
    std::string ip = primary.ip();
    int port = primary.port();

    LOG(ERROR) << "redirect to primary:" << ip << " port:" << port;
    auto client = std::make_unique<NetChannel>(ip, port);
    if (client->SendRawMessage(*request) == 0) {
      QueryResponse resp;
      if (client->RecvRawMessage(&resp) == 0) {
        if (context != nullptr && context->client != nullptr) {
          LOG(ERROR) << "send response from primary:"
                     << resp.transactions_size();
          int ret = context->client->SendRawMessage(resp);
          if (ret) {
            LOG(ERROR) << "send resp fail ret:" << ret;
          }
        }
      }
    }
    return 0;
  }

  QueryRequest query;
  if (!query.ParseFromString(request->data())) {
    LOG(ERROR) << "parse data fail";
    return -2;
  }

  QueryResponse response;
  if (query.max_seq() == 0 && query.min_seq() == 0) {
    uint64_t mseq = recovery_->GetMaxSeq();
    response.set_max_seq(mseq);
    LOG(ERROR) << "get max seq:" << mseq;
  } else {
    auto res =
        recovery_->GetDataFromRecoveryFiles(query.min_seq(), query.max_seq());

    for (const auto& it : res) {
      for (const auto& req : it.second) {
        Request* ret_request = req.second.get();

        Request* txn = response.add_transactions();
        txn->set_data(ret_request->data());
        txn->set_hash(ret_request->hash());
        txn->set_seq(ret_request->seq());
        txn->set_proxy_id(ret_request->proxy_id());
      }
    }
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
