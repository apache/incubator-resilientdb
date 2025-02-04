#include "service/contract/executor/x_manager/seq_controller.h"

#include <glog/logging.h>

#include <queue>
#include <stack>

#include "common/utils/utils.h"
#include "eEVM/exception.h"

//#define CDebug
//#define DDebug

namespace resdb {
namespace contract {
namespace x_manager {

SeqController::SeqController(DataStorage* storage, int window_size)
    : ConcurrencyController(storage),
      window_size_(window_size),
      storage_(storage) {
  assert(window_size + 1 <= 8192);
  changes_list_.resize(window_size + 1);
  Clear();
}

SeqController::~SeqController() {}

void SeqController::Clear() {
  for (int i = 0; i < window_size_; i++) {
    changes_list_[i].clear();
  }
}

// ========================================================
const ModifyMap& SeqController::GetChangeList(int64_t commit_id) {
  return changes_list_[commit_id];
}

void SeqController::Store(const int64_t commit_id, const uint256_t& address,
                          const uint256_t& value, int version) {
  storage_->Store(address, value, false);
  Data data = Data(STORE, value);
  auto& change_set = changes_list_[commit_id][address];
  change_set.push_back(data);
  // LOG(ERROR)<<" commit id:"<<commit_id<<" store addr:"<<address<<"
  // save:"<<value;
}

uint256_t SeqController::Load(const int64_t commit_id, const uint256_t& address,
                              int version) {
  auto& change_set = changes_list_[commit_id][address];
  uint256_t ret = storage_->Load(address, false).first;
  // LOG(ERROR)<<" commit id:"<<commit_id<<" load addr:"<<address<<" ret:"<<ret;
  Data data = Data(STORE, ret);
  change_set.push_back(data);
  return ret;
}

}  // namespace x_manager
}  // namespace contract
}  // namespace resdb
