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

#include "platform/consensus/execution/geo_transaction_executor.h"
#include "platform/consensus/ordering/geo_pbft/geo_pbft_commitment.h"
#include "platform/consensus/ordering/pbft/consensus_manager_pbft.h"

namespace resdb {

class ConsensusManagerGeoPBFT : public ConsensusManagerPBFT {
 public:
  ConsensusManagerGeoPBFT(
      const ResDBConfig& config,
      std::unique_ptr<GeoTransactionExecutor> local_executor,
      std::unique_ptr<GeoGlobalExecutor> global_executor);
  virtual ~ConsensusManagerGeoPBFT() = default;

  int ConsensusCommit(std::unique_ptr<Context> context,
                      std::unique_ptr<Request> request) override;

 private:
  std::unique_ptr<GeoPBFTCommitment> commitment_;
};

}  // namespace resdb
