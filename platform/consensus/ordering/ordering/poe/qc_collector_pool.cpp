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

#include "platform/consensus/ordering/poe/qc_collector_pool.h"

#include <glog/logging.h>

namespace resdb {

namespace {
uint32_t GetCapacity(uint32_t size) {
  int i = 0;
  for (i = 0; (1u << i) <= size; ++i)
    ;
  return (1u << i);
}
}  // namespace

QCCollectorPool::QCCollectorPool(const std::string& name, uint32_t size)
    : name_(name),
      capacity_(GetCapacity(size * 2)),
      mask_((capacity_ << 1) - 1) {
  collector_.resize(capacity_ << 1);
  for (size_t i = 0; i < (capacity_ << 1); ++i) {
    collector_[i] = std::make_unique<QCCollector>(i);
  }
  LOG(ERROR) << "name:" << name_ << " create pool done. capacity:" << capacity_;
}

void QCCollectorPool::Update(uint64_t seq) {
  uint32_t idx = seq & mask_;
  if (collector_[idx]->Seq() != seq) {
    LOG(ERROR) << "seq not match, skip update:" << seq;
    return;
  }
  // LOG(ERROR)<<"update:"<<(idx^capacity_)<<" update seq:"<<seq+capacity_<< "
  // cap:"<<capacity_<<" seq:"<<seq;
  collector_[idx ^ capacity_] = std::make_unique<QCCollector>(seq + capacity_);
}

QCCollector* QCCollectorPool::GetCollector(uint64_t seq) {
  uint32_t idx = seq & mask_;
  return collector_[idx].get();
}

}  // namespace resdb
