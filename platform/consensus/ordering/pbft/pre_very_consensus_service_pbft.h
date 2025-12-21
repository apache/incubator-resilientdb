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

#include "platform/consensus/ordering/pbft/consensus_service_pbft.h"

namespace resdb {

template <typename RequestType>
class PreVerifyConsensusServicePBFT : public ConsensusServicePBFT {
 public:
  PreVerifyConsensusServicePBFT(
      const ResDBConfig& config,
      std::unique_ptr<TransactionExecutorImpl> executor)
      : ConsensusServicePBFT(config, std::move(executor)) {
    SetPreVerifyFunc();
  }
  virtual ~PreVerifyConsensusServicePBFT() = default;

  void SetPreVerifyFunc();

  virtual bool VerifyClientRequest(const RequestType& request) = 0;
};

template <typename RequestType>
void PreVerifyConsensusServicePBFT<RequestType>::SetPreVerifyFunc() {
  ConsensusServicePBFT::SetPreVerifyFunc([&](const Request& request) {
    RequestType user_request;
    if (!user_request.ParseFromString(request.data())) {
      LOG(ERROR) << "parse data fail";
      return false;
    }

    return VerifyClientRequest(user_request);
  });
}

}  // namespace resdb
