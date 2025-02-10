#include "executor/contract/manager/global_state.h"

#include <glog/logging.h>

namespace resdb {
namespace contract {

using eevm::AccountState;
using eevm::Address;
using eevm::Code;
using eevm::SimpleAccount;


uint256_t AccountToAddress(const eevm::Address& account) {
    std::vector<uint8_t> code;
    code.resize(64);
    eevm::to_big_endian(account, code.data());
    code[63]=1;

    uint8_t h[32];
    eevm::keccak_256(code.data(), static_cast<unsigned int>(64), h);
    return eevm::from_big_endian(h, sizeof(h));
}

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

std::string GlobalState::GetBalance(const eevm::Address& account) {
  return storage_->GetValue(eevm::to_hex_string(AccountToAddress(account)));
}

int GlobalState::SetBalance(const eevm::Address& account, const uint256_t& balance) {
  return storage_->SetValue(eevm::to_hex_string(AccountToAddress(account)), eevm::to_hex_string(balance));
}

}  // namespace contract
}  // namespace resdb
