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

#include "executor/common/custom_query.h"
#include "executor/common/transaction_manager.h"
#include "executor/utxo/manager/transaction.h"
#include "executor/utxo/manager/wallet.h"
#include "platform/config/resdb_config_utils.h"
#include "proto/utxo/config.pb.h"

namespace resdb {
namespace utxo {

class UTXOExecutor : public TransactionManager {
 public:
  UTXOExecutor(const Config& config, Transaction* transaction, Wallet* wallet);
  ~UTXOExecutor();

  std::unique_ptr<std::string> ExecuteData(const std::string& request) override;

 private:
  Transaction* transaction_;
};

class QueryExecutor : public CustomQuery {
 public:
  QueryExecutor(Transaction* transaction, Wallet* wallet);
  virtual ~QueryExecutor() = default;

  virtual std::unique_ptr<std::string> Query(
      const std::string& request_str) override;

 private:
  Transaction* transaction_;
  Wallet* wallet_;
};

}  // namespace utxo
}  // namespace resdb
