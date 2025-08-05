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

#pragma once

#include <map>
#include <optional>
#include <unordered_map>

#include "service/kv_service/py_verificator.h"

#include "chain/state/chain_state.h"
#include "executor/common/transaction_manager.h"
#include "platform/config/resdb_config_utils.h"

namespace sdk {

class KVServiceTransactionManager : public resdb::TransactionManager {
public:
  KVServiceTransactionManager(std::unique_ptr<resdb::ChainState> state);
  virtual ~KVServiceTransactionManager() = default;

  std::unique_ptr<std::string> ExecuteData(const std::string &request) override;

protected:
  virtual void Set(const std::string &key, const std::string &value);
  std::string Get(const std::string &key);
  std::string GetAllValues();
  std::string GetRange(const std::string &min_key, const std::string &max_key);

private:
  std::unique_ptr<resdb::ChainState> state_;
  std::unique_ptr<PYVerificator> py_verificator_;
};

} // namespace sdk
