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

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "chain/storage/proto/ipfs_config.pb.h"

namespace resdb {
namespace storage {

class IPFSClient {
 public:
  virtual ~IPFSClient() = default;

  virtual std::string Add(const std::string& data) = 0;
  virtual std::string AddDAG(const std::string& json_data) = 0;
  virtual std::string Cat(const std::string& cid) = 0;
  virtual std::string GetDAG(const std::string& cid) = 0;
  virtual bool Exists(const std::string& cid) = 0;
  virtual bool IsEnabled() const = 0;

  static std::unique_ptr<IPFSClient> Create(
      const IPFSConfig& config);
};

std::unique_ptr<IPFSClient> NewIPFSClient(
    const std::string& api_endpoint,
    bool enabled = true);

}  // namespace storage
}  // namespace resdb