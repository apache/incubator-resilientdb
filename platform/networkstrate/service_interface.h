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

#include <atomic>
#include <memory>

#include "platform/common/data_comm/data_comm.h"
#include "platform/networkstrate/server_comm.h"

namespace resdb {

class ServiceInterface {
 public:
  ServiceInterface() : is_running_(false) {}
  virtual ~ServiceInterface() = default;

  virtual int Process(std::unique_ptr<Context> context,
                      std::unique_ptr<DataInfo> request_info);
  virtual bool IsRunning() const;
  virtual bool IsReady() const { return false; }
  virtual void SetRunning(bool is_running);
  virtual void Start();
  virtual void Stop();

 private:
  std::atomic<bool> is_running_;
};

}  // namespace resdb
