#include "service/contract/executor/paral_sm/db_view.h"

#include <glog/logging.h>

#include "eEVM/util.h"

namespace resdb {
namespace contract {
namespace paral_sm {

DBView::DBView(ConcurrencyController* controller, int64_t commit_id,
               int version)
    : controller_(static_cast<StreamingEController*>(controller)),
      commit_id_(commit_id),
      version_(version) {}

void DBView::store(const uint256_t& key, const uint256_t& value) {
  controller_->Store(commit_id_, key, value, version_);
}

uint256_t DBView::load(const uint256_t& key) {
  return controller_->Load(commit_id_, key, version_);
}

bool DBView::remove(const uint256_t& key) {
  // LOG(ERROR)<<"remove key:"<<key;
  return controller_->Remove(commit_id_, key, version_);
}

/*
void DBView::Flesh(int64_t commit_id) {
  commit_id_ = commit_id;
  //LOG(ERROR)<<"commit push:"<<commit_id;
  controller_->PushCommit(commit_id, local_changes_);
  local_changes_.clear();
}
*/

}  // namespace paral_sm
}  // namespace contract
}  // namespace resdb
