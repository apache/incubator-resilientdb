#include "executor/contract/manager/global_view.h"

#include <glog/logging.h>

#include "eEVM/util.h"

namespace resdb {
namespace contract {

GlobalView::GlobalView(resdb::Storage* storage) : storage_(storage) {}

void GlobalView::store(const uint256_t& key, const uint256_t& value) {
  storage_->SetValue(eevm::to_hex_string(key), eevm::to_hex_string(value));
}

uint256_t GlobalView::load(const uint256_t& key) {
  return eevm::to_uint256(storage_->GetValue(eevm::to_hex_string(key)));
}

bool GlobalView::remove(const uint256_t& key) { return true; }

}  // namespace contract
}  // namespace resdb
