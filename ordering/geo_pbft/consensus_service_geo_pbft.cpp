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

#include "ordering/geo_pbft/consensus_service_geo_pbft.h"

namespace resdb {

ConsensusServiceGeoPBFT::ConsensusServiceGeoPBFT(
    const ResDBConfig& config,
    std::unique_ptr<GeoTransactionExecutor> local_executor,
    std::unique_ptr<GeoGlobalExecutor> global_executor)
    : ConsensusServicePBFT(config, std::move(local_executor)) {
  commitment_ = std::make_unique<GeoPBFTCommitment>(
      std::move(global_executor), config, std::make_unique<SystemInfo>(config),
      GetBroadCastClient(), GetSignatureVerifier());
  ConsensusServicePBFT::SetNeedCommitQC(true);
}

int ConsensusServiceGeoPBFT::ConsensusCommit(std::unique_ptr<Context> context,
                                             std::unique_ptr<Request> request) {
  switch (request->type()) {
    case Request::TYPE_GEO_REQUEST:
      return commitment_->GeoProcessCcm(std::move(context), std::move(request));
  }
  return ConsensusServicePBFT::ConsensusCommit(std::move(context),
                                               std::move(request));
}

}  // namespace resdb
