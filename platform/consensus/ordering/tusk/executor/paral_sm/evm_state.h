#pragma once

#include "eEVM/globalstate.h"

namespace resdb {
namespace contract {

class EVMState : public eevm::GlobalState {
public:
  EVMState() = default;
  virtual ~EVMState() = default;

protected:
  const eevm::Block& get_current_block() override { return block_; }
  uint256_t get_block_hash(uint8_t offset) override { return 0; }

private:
  // Unused.
  eevm::Block block_;
};

}
} // namespace eevm
