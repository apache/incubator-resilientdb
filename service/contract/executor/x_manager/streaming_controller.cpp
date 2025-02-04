#include "service/contract/executor/x_manager/streaming_controller.h"

#include <glog/logging.h>

#include <queue>
#include <stack>

#include "common/utils/utils.h"
#include "eEVM/exception.h"

namespace resdb {
namespace contract {
namespace x_manager {

StreamingController::StreamingController(DataStorage* storage)
    : ConcurrencyController(storage), storage_(storage) {}

StreamingController::~StreamingController() {}

// ========================================================
void StreamingController::Store(const int64_t commit_id,
                                const uint256_t& address,
                                const uint256_t& value, int version) {
  // LOG(ERROR)<<" store:"<<address;
  storage_->StoreWithVersion(address, value, version, false);
}

uint256_t StreamingController::Load(const int64_t commit_id,
                                    const uint256_t& address, int version) {
  // LOG(ERROR)<<" load:"<<address;
  auto ret = storage_->Load(address, false);
  return ret.first;
}

bool StreamingController::Remove(const int64_t commit_id, const uint256_t& key,
                                 int version) {
  Store(commit_id, key, 0, version);
  return true;
}

}  // namespace x_manager
}  // namespace contract
}  // namespace resdb
