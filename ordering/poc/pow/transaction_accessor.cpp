#include "ordering/poc/pow/transaction_accessor.h"

#include <glog/logging.h>

namespace resdb {

TransactionAccessor::TransactionAccessor(const ResDBPoCConfig& config,
                                         bool auto_start)
    : config_(config) {
  stop_ = false;
  max_received_seq_ = 0;
  next_consume_ = 1;
  if (auto_start) {
    fetching_thread_ =
        std::thread(&TransactionAccessor::TransactionFetching, this);
  }
}

TransactionAccessor::~TransactionAccessor() {
  stop_ = true;
  if (fetching_thread_.joinable()) {
    fetching_thread_.join();
  }
}

void TransactionAccessor::Start() {
  fetching_thread_ =
      std::thread(&TransactionAccessor::TransactionFetching, this);
}

void TransactionAccessor::TransactionFetching() {
  std::unique_ptr<ResDBTxnClient> client = GetResDBTxnClient();

  assert(client != nullptr);
  while (!stop_) {
    uint64_t cur_seq = max_received_seq_ + 1;
    auto ret = client->GetTxn(cur_seq, cur_seq);
    if (!ret.ok() || (ret->size()) != 1 || (*ret)[0].first != cur_seq) {
      // LOG(ERROR) << "get txn fail:" << cur_seq;
      sleep(1);
      continue;
    }
    std::unique_ptr<ClientTransactions> client_txn =
        std::make_unique<ClientTransactions>();
    client_txn->set_transaction_data((*ret)[0].second);
    client_txn->set_seq(cur_seq);
    queue_.Push(std::move(client_txn));
    max_received_seq_ = cur_seq;
  }
  return;
}

std::unique_ptr<ResDBTxnClient> TransactionAccessor::GetResDBTxnClient() {
  return std::make_unique<ResDBTxnClient>(*config_.GetBFTConfig());
}

// obtain [seq, seq+batch_num-1] transactions
std::unique_ptr<BatchClientTransactions>
TransactionAccessor::ConsumeTransactions(uint64_t seq) {
  LOG(ERROR) << "consume transaction:" << seq
             << " batch:" << config_.BatchTransactionNum()
             << " received max seq:" << max_received_seq_;
  if (seq + config_.BatchTransactionNum() > max_received_seq_ + 1) {
    return nullptr;
  }
  if (seq != next_consume_) {
    LOG(ERROR) << "next should consume:" << next_consume_;
    return nullptr;
  }

  std::unique_ptr<BatchClientTransactions> batch_transactions =
      std::make_unique<BatchClientTransactions>();
  for (uint32_t i = 0; i < config_.BatchTransactionNum(); ++i) {
    *batch_transactions->add_transactions() = *queue_.Pop();
  }
  batch_transactions->set_min_seq(seq);
  batch_transactions->set_max_seq(seq + config_.BatchTransactionNum() - 1);
  next_consume_ = next_consume_ + config_.BatchTransactionNum();
  return batch_transactions;
}

}  // namespace resdb
