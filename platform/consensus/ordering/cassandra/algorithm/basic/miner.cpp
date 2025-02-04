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

#include "platform/consensus/ordering/cassandra/algorithm/basic/miner.h"

#include <assert.h>
#include <glog/logging.h>

#include <boost/format.hpp>
#include <thread>

#include "platform/consensus/ordering/cassandra/algorithm/basic/miner_utils.h"

namespace resdb {
namespace cassandra {
namespace basic {

Miner::Miner(int worker_num) : worker_num_(worker_num) {
  // the number of zeros ahead of the binary value.
  difficulty_ = 8;
  LOG(INFO) << " target config value: difficulty:" << difficulty_;
}

absl::StatusOr<int32_t> Miner::Mine(const Header& header) {
  LOG(ERROR) << " start mine block worker:" << worker_num_;
  stop_ = false;

  uint64_t min_slice = 0;
  uint64_t max_slice = 1ll << 31;

  std::vector<std::thread> ths;
  std::atomic<bool> solution_found = false;

  std::string header_str;
  header.SerializeToString(&header_str);
  std::string header_hash = GetHashValue(header_str);

  int32_t solution = 0;
  uint64_t step = static_cast<uint64_t>(worker_num_);
  for (uint32_t i = 0; i < worker_num_; ++i) {
    uint64_t current_slice_start = i + min_slice;
    ths.push_back(std::thread(
        [&](std::pair<uint64_t, uint64_t> slice) {
          for (uint64_t nonce = slice.first;
               nonce <= max_slice && !stop_ && !solution_found; nonce += step) {
            std::string str_value = header_hash + std::to_string(nonce);
            std::string hash_digest = GetHashValue(str_value);

            if (IsValidDigest(hash_digest, difficulty_)) {
              solution = nonce;
              solution_found = true;
              break;
            }
          }
          // LOG(ERROR) << "mine done slice:" << slice.first << " len:" << step;
        },
        std::make_pair(current_slice_start, 0)));
  }

  for (auto& th : ths) {
    if (th.joinable()) {
      th.join();
    }
  }
  if (stop_) {
    LOG(ERROR) << "minning has been terminated.";
    return absl::CancelledError("terminated");
  }

  if (solution_found) {
    LOG(ERROR) << "find solution:" << solution;
    return solution;
  }
  LOG(ERROR) << "solution not found";
  return absl::NotFoundError("solution not found");
}

void Miner::Terminate() {
  LOG(ERROR) << "terminate mining";
  stop_ = true;
}

bool Miner::Verify(const Header& header, int32_t nonce) {
  std::string header_str;
  header.SerializeToString(&header_str);
  std::string header_hash = GetHashValue(header_str);

  std::string str_value = header_hash + std::to_string(nonce);
  std::string hash_digest = GetHashValue(str_value);

  return IsValidDigest(hash_digest, difficulty_);
}

}  // namespace basic
}  // namespace cassandra
}  // namespace resdb
