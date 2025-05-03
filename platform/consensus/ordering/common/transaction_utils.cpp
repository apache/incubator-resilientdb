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

#include "platform/consensus/ordering/common/transaction_utils.h"

namespace resdb {

std::unique_ptr<Request> NewRequest(Request::Type type, const Request& request,
                                    int sender_id) {
  auto new_request = std::make_unique<Request>(request);
  new_request->set_type(type);
  new_request->set_sender_id(sender_id);
  return new_request;
}

std::unique_ptr<Request> NewRequest(Request::Type type, const Request& request,
                                    int sender_id, int region_id) {
  auto new_request = std::make_unique<Request>(request);
  new_request->set_type(type);
  new_request->set_sender_id(sender_id);
  new_request->mutable_region_info()->set_region_id(region_id);
  return new_request;
}

}  // namespace resdb
