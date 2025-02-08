#include "executor/contract/manager/global_state.h"

#include <glog/logging.h>

namespace resdb {
namespace contract {

using eevm::AccountState;
using eevm::Address;
using eevm::Code;
using eevm::SimpleAccount;

GlobalState::GlobalState(resdb::Storage* storage) : storage_(storage) {}

bool GlobalState::Exists(const eevm::Address& addr) {
  return accounts.find(addr) != accounts.cend();
}

void GlobalState::remove(const Address& addr) { accounts.erase(addr); }

AccountState GlobalState::get(const Address& addr) {
  const auto acc = accounts.find(addr);
  if (acc != accounts.cend()) return acc->second;

  return create(addr, 0, {});
}

AccountState GlobalState::create(const Address& addr, const uint256_t& balance,
                                 const Code& code) {
  Insert({SimpleAccount(addr, balance, code), GlobalView(storage_)});

  return get(addr);
}

const eevm::SimpleAccount& GlobalState::GetAccount(const eevm::Address& addr) {
  const auto acc = accounts.find(addr);
  return acc->second.first;
}

void GlobalState::Insert(const StateEntry& p) {
  const auto ib = accounts.insert(std::make_pair(p.first.get_address(), p));

  assert(ib.second);
}

}  // namespace contract
}  // namespace resdb
