#include "service/contract/executor/x_manager/local_view.h"

#include <glog/logging.h>

#include "eEVM/util.h"

namespace resdb {
namespace contract {
namespace x_manager {

LocalView::LocalView(ConcurrencyController* controller, int64_t commit_id)
    : controller_(controller), commit_id_(commit_id) {}

void LocalView::store(const uint256_t& key, const uint256_t& value) {
  // LOG(ERROR)<<"========= store key:"<<key<<" value:"<<value;
  auto it = local_changes_.find(key);
  if (it == local_changes_.end()) {
    std::pair<uint256_t, int64_t> load_value =
        controller_->GetStorage()->Load(key, /*is_from_local=*/true);
    local_changes_[key].push_back(
        Data(STORE, value, load_value.second, load_value.first));
  } else {
    const Data& data = local_changes_[key].back();
    if (data.state == LOAD) {
      local_changes_[key].push_back(Data(STORE, value, data.version + 1));
    } else {
      local_changes_[key].pop_back();
      local_changes_[key].push_back(Data(STORE, value, data.version));
    }
  }
}

uint256_t LocalView::load(const uint256_t& key) {
  // LOG(ERROR)<<"load key:"<<key;
  auto it = local_changes_.find(key);
  if (it == local_changes_.end()) {
    std::pair<uint256_t, int64_t> value =
        controller_->GetStorage()->Load(key, /*is_from_local=*/true);
    local_changes_[key].push_back(Data(LOAD, value.first, value.second));
    return value.first;
  }
  return it->second.back().data;
}

bool LocalView::remove(const uint256_t& key) {
  local_changes_[key].push_back(Data(REMOVE));
  store(key, 0);
  return true;
}

void LocalView::Flesh(int64_t commit_id) {
  commit_id_ = commit_id;
  // LOG(ERROR)<<"commit push:"<<commit_id<<"
  // local_changes_:"<<local_changes_.size();
  controller_->PushCommit(commit_id, local_changes_);
  local_changes_.clear();
}

}  // namespace x_manager
}  // namespace contract
}  // namespace resdb
