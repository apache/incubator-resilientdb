/*
 * Copyright (c) 2019-2022 ExpoLab, UC Davis
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#include "platform/consensus/ordering/poc/pow/transaction_accessor.h"

#include <glog/logging.h>

#include "common/utils/utils.h"

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
  std::unique_ptr<ResDBTxnAccessor> client = GetResDBTxnAccessor();
  assert(client != nullptr);
  while (!stop_) {
    uint64_t cur_seq = max_received_seq_ + 1;
    auto ret = client->GetTxn(cur_seq, cur_seq);
    if (!ret.ok() || (ret->size()) != 1 || (*ret)[0].first != cur_seq) {
      sleep(1);
      continue;
    }
    std::unique_ptr<ClientTransactions> client_txn =
        std::make_unique<ClientTransactions>();
    client_txn->set_transaction_data((*ret)[0].second);
    client_txn->set_seq(cur_seq);
    client_txn->set_create_time(GetCurrentTime());
    queue_.Push(std::move(client_txn));
    max_received_seq_ = cur_seq;

    std::lock_guard<std::mutex> lk(mutex_);
    cv_.notify_all();
  }
  return;
}

std::unique_ptr<ResDBTxnAccessor> TransactionAccessor::GetResDBTxnAccessor() {
  return std::make_unique<ResDBTxnAccessor>(*config_.GetBFTConfig());
}

// obtain [seq, seq+batch_num-1] transactions
std::unique_ptr<BatchClientTransactions>
TransactionAccessor::ConsumeTransactions(uint64_t seq) {
  LOG(ERROR) << "consume transaction:" << seq
             << " batch:" << config_.BatchTransactionNum()
             << " received max seq:" << max_received_seq_;
  if (seq + config_.BatchTransactionNum() > max_received_seq_ + 1) {
    std::unique_lock<std::mutex> lk(mutex_);
    cv_.wait_for(lk, std::chrono::seconds(1), [&] {
      return seq + config_.BatchTransactionNum() <= max_received_seq_ + 1;
    });
    return nullptr;
  }
  while (seq > next_consume_) {
    *queue_.Pop();
    next_consume_++;
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
