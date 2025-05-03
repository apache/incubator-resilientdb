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

#include "platform/consensus/ordering/geo_pbft/consensus_manager_geo_pbft.h"

namespace resdb {

ConsensusManagerGeoPBFT::ConsensusManagerGeoPBFT(
    const ResDBConfig& config,
    std::unique_ptr<GeoTransactionExecutor> local_executor,
    std::unique_ptr<GeoGlobalExecutor> global_executor)
    : ConsensusManagerPBFT(config, std::move(local_executor)) {
  commitment_ = std::make_unique<GeoPBFTCommitment>(
      std::move(global_executor), config, std::make_unique<SystemInfo>(config),
      GetBroadCastClient(), GetSignatureVerifier());
  ConsensusManagerPBFT::SetNeedCommitQC(true);
}

int ConsensusManagerGeoPBFT::ConsensusCommit(std::unique_ptr<Context> context,
                                             std::unique_ptr<Request> request) {
  switch (request->type()) {
    case Request::TYPE_GEO_REQUEST:
      return commitment_->GeoProcessCcm(std::move(context), std::move(request));
  }
  return ConsensusManagerPBFT::ConsensusCommit(std::move(context),
                                               std::move(request));
}

}  // namespace resdb
