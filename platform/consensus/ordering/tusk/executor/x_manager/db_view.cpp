#include "service/contract/executor/x_manager/db_view.h"

#include "eEVM/util.h"

#include <glog/logging.h>

namespace resdb {
namespace contract {
namespace x_manager {

DBView::DBView(ConcurrencyController * controller, int64_t commit_id, int version)
  :controller_(static_cast<StreamingEController*>(controller)),
  commit_id_(commit_id), version_(version){ 
  }

void DBView::store(const uint256_t& key, const uint256_t& value) {
  //LOG(ERROR)<<"store:"<<key<<" value:"<<value;
  controller_->Store(commit_id_, key, value, version_);
}

uint256_t DBView::load(const uint256_t& key) {
  //LOG(ERROR)<<"laod:"<<key;
  return controller_->Load(commit_id_, key, version_);
}

bool DBView::remove(const uint256_t& key) {
  //LOG(ERROR)<<"remove key:"<<key;
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


}}
} // namespace eevm
