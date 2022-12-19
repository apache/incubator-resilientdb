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

#include <vector>

#include "ordering/pbft/transaction_collector.h"

namespace resdb {

class LockFreeCollectorPool {
 public:
  LockFreeCollectorPool(const std::string& name, uint32_t size,
                        TransactionExecutor* executor,
                        bool enable_viewchange = false);

  TransactionCollector* GetCollector(uint64_t seq);
  void Update(uint64_t seq);

 private:
  std::string name_;
  uint32_t capacity_;
  uint32_t mask_;
  TransactionExecutor* executor_;
  std::vector<std::unique_ptr<TransactionCollector>> collector_;
  bool enable_viewchange_;
};

}  // namespace resdb
