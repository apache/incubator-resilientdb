#pragma once

#include <map>

#include "eEVM/storage.h"
#include "service/contract/executor/x_manager/data_storage.h"

namespace resdb {
namespace contract {

class GlobalView : public eevm::Storage {
 public:
  GlobalView(DataStorage* storage);
  virtual ~GlobalView() = default;

  void store(const uint256_t& key, const uint256_t& value) override;
  uint256_t load(const uint256_t& key) override;
  bool remove(const uint256_t& key) override;

 private:
  DataStorage* storage_;
};

}  // namespace contract
}  // namespace resdb
