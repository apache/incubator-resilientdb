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

#include "platform/consensus/ordering/common/message_manager_basic.h"
#include "platform/consensus/ordering/tusk/proto/tusk.pb.h"

namespace resdb {
namespace tusk {

class MessageManager : public MessageManagerBasic {
 public:
  MessageManager(const ResDBConfig& config,
                 std::unique_ptr<TransactionManager> data_impl,
                 SystemInfo* system_info);

  int Commit(std::unique_ptr<TuskRequest> tusk_request);
  bool IsCommitted(const TuskRequest& request);

  uint64_t GetCurrentView();

 private:
  std::mutex mutex_;
  std::set<std::pair<int64_t, int> > committed_;
};

}  // namespace tusk
}  // namespace resdb
