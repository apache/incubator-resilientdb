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
