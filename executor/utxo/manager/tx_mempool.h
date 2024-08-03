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

#include "absl/status/statusor.h"
#include "proto/utxo/utxo.pb.h"

namespace resdb {
namespace utxo {

class TxMempool {
 public:
  TxMempool();
  ~TxMempool();

  // Add a new utxo and return the transaction id.
  int64_t AddUTXO(const UTXO& utxo);

  // get the transfer value from a transantion "id" in its output[out_idx].
  // if the address not match or the transaction does not exist, return -1
  int64_t GetUTXOOutValue(int64_t id, int out_idx, const std::string& address);

  absl::StatusOr<UTXOOut> GetUTXO(int64_t id, int out_idx,
                                  const std::string& address);

  // Mark the output of a trans has been spent.
  // Return the out value.
  int64_t MarkSpend(int64_t id, int out_idx, const std::string& address);

  std::vector<UTXO> GetUTXO(int64_t end_idx, int num);

 private:
  std::map<int64_t, std::unique_ptr<UTXO> > txs_;
  std::atomic<int64_t> id_;
};

}  // namespace utxo
}  // namespace resdb
