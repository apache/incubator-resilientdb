#include "platform/consensus/ordering/poc/pow/consensus_service_pow.h"

#include "common/utils/utils.h"
#include "glog/logging.h"

namespace resdb {

ConsensusServicePoW::ConsensusServicePoW(const ResDBPoCConfig& config)
    : common::Consensus(config, nullptr) {
  miner_manager_ = std::make_unique<MinerManager>(config);
  pow_manager_ = std::make_unique<PoWManager>(config, GetBroadCastClient());
}

void ConsensusServicePoW::Start() {
  common::Consensus::Start();
  pow_manager_->Start();
}

ConsensusServicePoW::~ConsensusServicePoW() {}

std::vector<ReplicaInfo> ConsensusServicePoW::GetReplicas() {
  return miner_manager_->GetReplicas();
}

int ConsensusServicePoW::ProcessCustomConsensus(
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
      } else {
        LOG(ERROR) << "parse info fail";
      }
      break;
    }
  }

  return 0;
}

}  // namespace resdb
