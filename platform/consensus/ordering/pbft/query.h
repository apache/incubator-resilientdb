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

#include "executor/common/custom_query.h"
#include "platform/config/resdb_config.h"
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
