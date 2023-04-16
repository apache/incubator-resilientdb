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

#include <map>
#include <optional>
#include <unordered_map>

#include "platform/config/resdb_config_utils.h"
#include "platform/consensus/execution/transaction_manager.h"
#include "storage/storage.h"

namespace resdb {

  // **needs a batched transactions param**
  // we only have get, set, batched writes
  // create the queue, check if there are things in the queue
  // if there is 1+ things in the queue, process
  // depending on how many things are in the queue, use 
  // up however many planner/execution threads
  // components:
  // - planner threads
  //    - priority groups (based on order of txns)
  // - execution threads
  //    - check if EQ is full, if so, then load balance
  //    - to load balance, split the EQ in half, then assign new 
  //    - EQ's to each half-range
  //    - use thread pool of EQ's to reduce cost
  // - execution priority invariance
  // - use EQ switch if there is intra-transaction dependency -> switch to 
  // - the other EQ that has the dependency
  // - undo buffer for recovery

class QueCC_Thread {
  public:
    QueCC_Thread();
    virtual ~QueCC_Thread();

  private:
    struct QueCC_ThreadPool {};
    struct QueCC_PlannerThread {};
    struct QueCC_ExecutionThread {};
    struct QueCC_PriorityGroup {};
};


class KVServiceTransactionManager : public TransactionManager {
 public:
  KVServiceTransactionManager(std::unique_ptr<Storage> storage);
  KVServiceTransactionManager(const ResConfigData& config_data,
                              char* cert_file);
  virtual ~KVServiceTransactionManager() = default;

  std::unique_ptr<std::string> ExecuteData(const std::string& request) override;

 protected:
  virtual void Set(const std::string& key, const std::string& value);
  std::string Get(const std::string& key);
  std::string GetValues();
  std::string GetRange(const std::string& min_key, const std::string& max_key);

 private:
  std::unique_ptr<Storage> storage_;
};

}  // namespace resdb
