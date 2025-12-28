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

#include "platform/config/resdb_poc_config.h"
#include "platform/consensus/ordering/common/framework/consensus.h"
#include "platform/consensus/ordering/poc/pow/miner_manager.h"
#include "platform/consensus/ordering/poc/pow/pow_manager.h"
#include "platform/networkstrate/consensus_manager.h"

namespace resdb {

class ConsensusServicePoW : public common::Consensus {
 public:
  ConsensusServicePoW(const ResDBPoCConfig& config);
  virtual ~ConsensusServicePoW();

  // Start the service.
  void Start() override;

  int ProcessCustomConsensus(std::unique_ptr<Request> request) override;
  std::vector<ReplicaInfo> GetReplicas() override;

 protected:
  std::unique_ptr<PoWManager> pow_manager_;
  std::unique_ptr<MinerManager> miner_manager_;
};

}  // namespace resdb
