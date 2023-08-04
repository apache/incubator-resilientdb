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

#include "platform/consensus/ordering/poc/pow/consensus_manager_pow.h"

#include <glog/logging.h>

#include "common/utils/utils.h"

namespace resdb {

ConsensusManagerPoW::ConsensusManagerPoW(const ResDBPoCConfig& config)
    : ConsensusManager(config) {
  miner_manager_ = std::make_unique<MinerManager>(config);
  pow_manager_ = std::make_unique<PoWManager>(config, GetBroadCastClient());
}

ConsensusManagerPoW::~ConsensusManagerPoW() {}

void ConsensusManagerPoW::Start() {
  ConsensusManager::Start();
  pow_manager_->Start();
}

std::vector<ReplicaInfo> ConsensusManagerPoW::GetReplicas() {
  return miner_manager_->GetReplicas();
}

int ConsensusManagerPoW::ConsensusCommit(std::unique_ptr<Context> context,
                                         std::unique_ptr<Request> request) {
  LOG(ERROR) << "recv impl type:" << request->type() << " "
             << request->client_info().DebugString()
             << "sender id:" << request->sender_id();
  switch (request->type()) {
    case PoWRequest::TYPE_COMMITTED_BLOCK: {
      std::unique_ptr<Block> block = std::make_unique<Block>();
      if (block->ParseFromString(request->data())) {
        pow_manager_->Commit(std::move(block));
      }
      break;
    }
    case PoWRequest::TYPE_SHIFT_MSG: {
      std::unique_ptr<SliceInfo> slice_info = std::make_unique<SliceInfo>();
      if (slice_info->ParseFromString(request->data())) {
        pow_manager_->AddShiftMsg(*slice_info);
      }
      break;
    }
  }
  return 0;
}

}  // namespace resdb
