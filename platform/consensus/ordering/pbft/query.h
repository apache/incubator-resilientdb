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

#include "platform/config/resdb_config.h"
#include "platform/consensus/execution/custom_query.h"
#include "platform/consensus/ordering/pbft/message_manager.h"

namespace resdb {

class Query {
 public:
  Query(const ResDBConfig& config, MessageManager* message_manager,
        std::unique_ptr<CustomQuery> executor = nullptr);
  virtual ~Query();

  virtual int ProcessGetReplicaState(std::unique_ptr<Context> context,
                                     std::unique_ptr<Request> request);
  virtual int ProcessQuery(std::unique_ptr<Context> context,
                           std::unique_ptr<Request> request);

  virtual int ProcessCustomQuery(std::unique_ptr<Context> context,
                                 std::unique_ptr<Request> request);

 protected:
  ResDBConfig config_;
  MessageManager* message_manager_;
  std::unique_ptr<CustomQuery> custom_query_executor_;
};

}  // namespace resdb
