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