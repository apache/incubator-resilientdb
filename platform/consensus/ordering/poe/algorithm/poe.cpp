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

#include "platform/consensus/ordering/poe/algorithm/poe.h"

#include <glog/logging.h>

#include "common/crypto/signature_verifier.h"
#include "common/utils/utils.h"

namespace resdb {
namespace poe {

PoE::PoE(int id, int f, int total_num, SignatureVerifier* verifier)
    : ProtocolBase(id, f, total_num), verifier_(verifier) {
  LOG(ERROR) << "get proposal graph";
  id_ = id;
  total_num_ = total_num;
  f_ = f;
  is_stop_ = false;
  seq_ = 0;
}

PoE::~PoE() { is_stop_ = true; }

bool PoE::IsStop() { return is_stop_; }

bool PoE::ReceiveTransaction(std::unique_ptr<Transaction> txn) {
  // LOG(ERROR)<<"recv txn:";
  txn->set_create_time(GetCurrentTime());
  txn->set_seq(seq_++);
  txn->set_proposer(id_);

  Broadcast(MessageType::Propose, *txn);
  return true;
}

bool PoE::ReceivePropose(std::unique_ptr<Transaction> txn) {
  std::string hash = txn->hash();
  int64_t seq = txn->seq();
  int proposer = txn->proposer();
  {
    std::unique_lock<std::mutex> lk(mutex_);
    data_[txn->hash()] = std::move(txn);
  }

  Proposal proposal;
  proposal.set_hash(hash);
  proposal.set_seq(seq);
  proposal.set_proposer(id_);
  Broadcast(MessageType::Prepare, proposal);
  return true;
}

bool PoE::ReceivePrepare(std::unique_ptr<Proposal> proposal) {
  std::unique_ptr<Transaction> txn = nullptr;
  {
    std::unique_lock<std::mutex> lk(mutex_);
    received_[proposal->hash()].insert(proposal->proposer());
    auto it = data_.find(proposal->hash());
    if (it != data_.end()) {
      if (received_[proposal->hash()].size() >= 2 * f_ + 1) {
        txn = std::move(it->second);
        data_.erase(it);
      }
    }
  }
  if (txn != nullptr) {
    commit_(*txn);
  }
  return true;
}

}  // namespace poe
}  // namespace resdb
