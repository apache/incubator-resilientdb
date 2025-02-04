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

#include <iostream>
#include <mutex>
#include <set>

namespace resdb {

template <class _type>
class SpinLockSet {
 public:
  SpinLockSet(){};
  ~SpinLockSet() { set_.clear(); }

  int exists(_type key) {
    bool result = false;
    lock_.lock();
    result = set_.count(key);
    lock_.unlock();
    return result;
  }
  void add(_type key) {
    lock_.lock();
    set_.insert(key);
    lock_.unlock();
  };
  int check_and_add(_type key) {
    bool result = false;
    lock_.lock();
    result = set_.count(key);
    set_.insert(key);
    lock_.unlock();
    return result;
  }
  int remove(_type key) {
    lock_.lock();

    int result = set_.erase(key);

    lock_.unlock();
    return result;
  }

  int size() { return set_.size(); }

 private:
  std::set<_type> set_;
  std::mutex lock_;
};

}  // namespace resdb