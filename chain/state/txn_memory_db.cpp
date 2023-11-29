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

<<<<<<< HEAD:chain/state/chain_state.cpp
#include "chain/state/chain_state.h"
=======
#include "chain/state/txn_memory_db.h"
>>>>>>> 041507eb4a569c4aad071bbe47f2953b4c9e4363:chain/state/txn_memory_db.cpp

#include <glog/logging.h>

namespace resdb {

ChainState::ChainState() : max_seq_(0) {}

Request* ChainState::Get(uint64_t seq) {
  std::unique_lock<std::mutex> lk(mutex_);
  if (data_.find(seq) == data_.end()) {
    return nullptr;
  }
  return data_[seq].get();
}

void ChainState::Put(std::unique_ptr<Request> request) {
  std::unique_lock<std::mutex> lk(mutex_);
  max_seq_ = request->seq();
  data_[max_seq_] = std::move(request);
}

uint64_t ChainState::GetMaxSeq() { return max_seq_; }

}  // namespace resdb
