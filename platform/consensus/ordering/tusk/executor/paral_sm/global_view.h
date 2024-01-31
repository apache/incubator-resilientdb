#pragma once

#include <map>

#include "service/contract/executor/paral_sm/data_storage.h"
#include "eEVM/storage.h"

namespace resdb {
namespace contract {
namespace paral_sm {

class GlobalView : public eevm::Storage {

public:
  GlobalView(DataStorage* storage);
  virtual ~GlobalView() = default;

  void store(const uint256_t& key, const uint256_t& value) override;
  uint256_t load(const uint256_t& key) override;
  bool remove(const uint256_t& key) override;

private:
  DataStorage * storage_;
};

}
}
} // namespace resdb
