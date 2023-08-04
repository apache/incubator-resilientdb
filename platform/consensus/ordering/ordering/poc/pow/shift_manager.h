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

#pragma once

#include <condition_variable>

#include "platform/config/resdb_poc_config.h"
#include "platform/consensus/ordering/poc/proto/pow.pb.h"

namespace resdb {

class ShiftManager {
 public:
  ShiftManager(const ResDBPoCConfig& config);
  virtual ~ShiftManager() = default;

  void AddSliceInfo(const SliceInfo& slice_info);
  virtual bool Check(const SliceInfo& slice_info, int timeout_ms = 10000);

 private:
  ResDBPoCConfig config_;
  std::map<std::pair<uint64_t, uint64_t>, std::set<uint32_t>> data_;
  std::mutex mutex_;
  std::condition_variable cv_;
};

}  // namespace resdb
