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

#include "platform/networkstrate/service_interface.h"

#include <glog/logging.h>

namespace resdb {

int ServiceInterface::Process(std::unique_ptr<Context> context,
                              std::unique_ptr<DataInfo> request_info) {
  return 0;
}

bool ServiceInterface::IsRunning() const { return is_running_; }

void ServiceInterface::SetRunning(bool is_running) { is_running_ = is_running; }

void ServiceInterface::Start() { SetRunning(true); }
void ServiceInterface::Stop() { SetRunning(false); }

}  // namespace resdb
