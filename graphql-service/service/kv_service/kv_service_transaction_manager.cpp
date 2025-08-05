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

#include "service/kv_service/kv_service_transaction_manager.h"

#include <glog/logging.h>

#include "chain/state/chain_state.h"
#include "service/kv_service/proto/kv_server.pb.h"

#ifdef ENABLE_LEVELDB
#include "chain/storage/res_leveldb.h"
#endif

#ifdef ENABLE_ROCKSDB
#include "chain/storage/res_rocksdb.h"
#endif

namespace sdk {

using resdb::ChainState;
using resdb::ResConfigData;

KVServiceTransactionManager::KVServiceTransactionManager(
    std::unique_ptr<ChainState> state)
    : state_(std::move(state)) {}

std::unique_ptr<std::string>
KVServiceTransactionManager::ExecuteData(const std::string &request) {
  KVRequest kv_request;
  KVResponse kv_response;

  if (!kv_request.ParseFromString(request)) {
    LOG(ERROR) << "parse data fail";
    return nullptr;
  }

  if (kv_request.cmd() == KVRequest::SET) {
    Set(kv_request.key(), kv_request.value());
  } else if (kv_request.cmd() == KVRequest::GET) {
    kv_response.set_value(Get(kv_request.key()));
  } else if (kv_request.cmd() == KVRequest::GETALLVALUES) {
    kv_response.set_value(GetAllValues());
  } else if (kv_request.cmd() == KVRequest::GETRANGE) {
    kv_response.set_value(GetRange(kv_request.key(), kv_request.value()));
  }

  std::unique_ptr<std::string> resp_str = std::make_unique<std::string>();
  if (!kv_response.SerializeToString(resp_str.get())) {
    return nullptr;
  }

  return resp_str;
}

void KVServiceTransactionManager::Set(const std::string &key,
                                      const std::string &value) {
  bool is_valid = py_verificator_->Validate(value);
  if (!is_valid) {
    LOG(ERROR) << "Invalid transaction for " << key;
    return;
  }
  state_->SetValue(key, value);
}

std::string KVServiceTransactionManager::Get(const std::string &key) {
  return state_->GetValue(key);
}

std::string KVServiceTransactionManager::GetAllValues() {
  return state_->GetAllValues();
}

// Get values on a range of keys
std::string KVServiceTransactionManager::GetRange(const std::string &min_key,
                                                  const std::string &max_key) {
  return state_->GetRange(min_key, max_key);
}

} // namespace sdk
