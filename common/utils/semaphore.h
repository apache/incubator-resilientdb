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

#include <condition_variable>
#include <mutex>

class Semaphore {
 public:
  explicit Semaphore(int count) : count_(count) {}

  void Acquire() {
    std::unique_lock<std::mutex> lk(mutex_);
    cv_.wait(lk, [this] { return count_ > 0; });
    count_--;
  }

  void Release() {
    std::unique_lock<std::mutex> lk(mutex_);
    count_++;
    cv_.notify_one();
  }

 private:
  std::mutex mutex_;
  std::condition_variable cv_;
  int count_;
};
