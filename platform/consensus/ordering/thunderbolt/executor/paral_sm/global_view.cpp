#include "service/contract/executor/paral_sm/global_view.h"

#include <glog/logging.h>

#include "eEVM/util.h"

namespace resdb {
namespace contract {
namespace paral_sm {

GlobalView::GlobalView(DataStorage* storage) : storage_(storage) {}

void GlobalView::store(const uint256_t& key, const uint256_t& value) {
  storage_->Store(key, value);
}

uint256_t GlobalView::load(const uint256_t& key) {
  return storage_->Load(key).first;
}

bool GlobalView::remove(const uint256_t& key) { return storage_->Remove(key); }

}  // namespace paral_sm
}  // namespace contract
}  // namespace resdb
