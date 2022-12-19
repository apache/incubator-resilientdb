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

#include <memory>

#include "proto/resdb.pb.h"

namespace resdb {

class TransactionExecutorImpl {
 public:
  TransactionExecutorImpl(bool is_out_of_order = false,
                          bool need_response = true);
  virtual ~TransactionExecutorImpl() = default;

  virtual std::unique_ptr<BatchClientResponse> ExecuteBatch(
      const BatchClientRequest& request);

  virtual std::unique_ptr<std::string> ExecuteData(const std::string& request);

  bool IsOutOfOrder();

  bool NeedResponse();

 private:
  bool is_out_of_order_ = false;
  bool need_response_ = true;
};
}  // namespace resdb
