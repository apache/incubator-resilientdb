/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
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

uint64_t GetCurrentTime() {
  uint64_t ret = 0;
  struct timeval tv;
  struct timezone tz;
  gettimeofday(&tv, &tz);
  return (uint64_t)(tv.tv_sec) * 1000000 + tv.tv_usec;
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

  uint64_t last_time = 0;
  uint64_t current_time = GetCurrentTime();
  sleep(10);
  int cur_seq_pbft = 1;
  srand(12345);

  while (!stop_) {
    uint64_t cur_seq = max_received_seq_ + 1;
    if (cur_seq > 10000000) break;
    auto ret = client->GetTxn(cur_seq, cur_seq + 1000);
    if (!ret.ok() || (*ret).size() == 0 || (*ret)[0].first != cur_seq) {
      // LOG(ERROR) << "get txn fail:" << cur_seq;
      sleep(1);
      continue;
    }

    double need_time = 2.0 / 1000;
    uint64_t current_time1 = GetCurrentTime();
    for (auto& res : (*ret)) {
      uint64_t current_time = GetCurrentTime();

      if (last_time > 0) {
        double sleep_time =
            std::max((uint64_t)0, current_time - last_time) / 1000000.0;
        //	LOG(ERROR)<<"sleep:"<<sleep_time<<" "<<(need_time-sleep_time)<<"
        //"<<(int)((need_time - sleep_time)*1000000);
        if (need_time - sleep_time > 0) {
          usleep((int)((need_time - sleep_time) * 1000000));
        }
      }

      // LOG(ERROR)<<"current time:"<<current_time;
      last_time = current_time;
      std::unique_ptr<ClientTransactions> client_txn =
          std::make_unique<ClientTransactions>();
      client_txn->set_transaction_data(res.second);
      client_txn->set_seq(cur_seq);
      client_txn->set_create_time(GetCurrentTime());
      // prometheus_handler_->Inc(TRANSACTION_INPUT, 1);
      queue_.Push(std::move(client_txn));
      cur_seq++;
    }
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
  LOG(ERROR) << "get batch:" << GetCurrentTime();
  return batch_transactions;
}

}  // namespace resdb
