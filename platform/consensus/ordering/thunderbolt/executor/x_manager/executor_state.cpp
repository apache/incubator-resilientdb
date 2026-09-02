#include "service/contract/executor/x_manager/executor_state.h"

#include <glog/logging.h>

namespace resdb {
namespace contract {
namespace x_manager {

using eevm::AccountState;
using eevm::Address;
using eevm::Code;
using eevm::SimpleAccount;

ExecutorState::ExecutorState(ConcurrencyController* controller,
                             int64_t commit_id)
    : controller_(controller) {}

bool ExecutorState::Exists(const eevm::Address& addr) {
  return accounts.find(addr) != accounts.cend();
}

void ExecutorState::remove(const Address& addr) { accounts.erase(addr); }

AccountState ExecutorState::get(const Address& addr) {
  const auto acc = accounts.find(addr);
  if (acc != accounts.cend()) return acc->second;

  return create(addr, 0, {});
}

AccountState ExecutorState::create(const Address& addr,
                                   const uint256_t& balance, const Code& code) {
  Insert({SimpleAccount(addr, balance, code), DBView(controller_, 0, 0)});
  return get(addr);
}

const eevm::SimpleAccount& ExecutorState::GetAccount(
    const eevm::Address& addr) {
  const auto acc = accounts.find(addr);
  return acc->second.first;
}

void ExecutorState::Set(const eevm::SimpleAccount& acc, int64_t commit_id,
                        int version) {
  Insert({acc, DBView(controller_, commit_id, version)});
}

void ExecutorState::Insert(const StateEntry& p) {
  const auto ib = accounts.insert(std::make_pair(p.first.get_address(), p));

  assert(ib.second);
}

bool ExecutorState::Flesh(const Address& addr, int commit_id) {
  const auto acc = accounts.find(addr);
  if (acc != accounts.cend()) {
    acc->second.second.Flesh(commit_id);
    return true;
  }
  return false;
}

/*
  bool ExecutorState::Commit(const eevm::Address& addr) {
    const auto acc = accounts.find(addr);
    if (acc != accounts.cend()){
      return acc->second.second.Commit();
    }
    return false;
  }
  */

}  // namespace x_manager
}  // namespace contract
}  // namespace resdb
