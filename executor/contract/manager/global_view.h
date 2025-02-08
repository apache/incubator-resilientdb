#pragma once

#include <map>

#include "eEVM/storage.h"
#include "chain/storage/storage.h"

namespace resdb {
namespace contract {

class GlobalView : public eevm::Storage {
 public:
  GlobalView(resdb::Storage* storage);
  virtual ~GlobalView() = default;

  void store(const uint256_t& key, const uint256_t& value) override;
  uint256_t load(const uint256_t& key) override;
  bool remove(const uint256_t& key) override;

 private:
  resdb::Storage* storage_;
};

}  // namespace contract
}  // namespace resdb
