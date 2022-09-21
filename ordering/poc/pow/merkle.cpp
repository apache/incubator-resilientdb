#include "ordering/poc/pow/merkle.h"

#include "ordering/poc/pow/miner_utils.h"

namespace resdb {
namespace {

std::string TrverseMakeHash(const BatchClientTransactions& transaction,
                            int l_idx, int r_idx) {
  if (l_idx == r_idx) {
    return GetHashValue(transaction.transactions(l_idx).transaction_data());
  }
  int mid = (l_idx + r_idx) >> 1;

  std::string l_chd = TrverseMakeHash(transaction, l_idx, mid);
  std::string r_chd = TrverseMakeHash(transaction, mid + 1, r_idx);
  return GetHashValue(l_chd + r_chd);
}

}  // namespace

HashValue Merkle::MakeHash(const BatchClientTransactions& transaction) {
  std::string root_hash =
      TrverseMakeHash(transaction, 0, transaction.transactions_size() - 1);
  return DigestToHash(root_hash);
}

}  // namespace resdb
