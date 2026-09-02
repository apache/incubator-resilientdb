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
#include "platform/consensus/ordering/thunderbolt/executor/manager/leveldb.h"

#include "glog/logging.h"

namespace resdb {
namespace contract {

LevelDB::LevelDB() {
  db_ = std::make_unique<ResLevelDB>("./");
  db_->SetBatchSize(10000);
}

void LevelDB::Flush() {
  // LOG(ERROR)<<"flush";
  for (const auto& it : s) {
    std::string addr = eevm::to_hex_string(it.first);
    std::string value = eevm::to_hex_string(it.second.first);
    // LOG(ERROR)<<"addr:"<<addr<<" value:"<<value;
    char* buf = new char[value.size() + sizeof(it.second.second)];
    memcpy(buf, &it.second.second, sizeof(it.second.second));
    memcpy(buf + sizeof(it.second.second), value.c_str(), value.size());

    db_->SetValue(addr,
                  std::string(buf, value.size() + sizeof(it.second.second)));
    delete buf;
  }
}

}  // namespace contract
}  // namespace resdb
