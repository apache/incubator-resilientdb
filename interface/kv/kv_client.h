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
#include <string>
#include <vector>

#include "interface/rdbc/transaction_constructor.h"
#include "proto/kv/kv.pb.h"

namespace resdb {

// KVClient to send data to the kv server.
class KVClient : public TransactionConstructor {
 public:
  KVClient(const ResDBConfig& config);

  // Version-based interfaces.
  // Obtain the current version before setting a new data
  int Set(const std::string& key, const std::string& data, int version);

  // Obtain the value with a specific version.
  // If the version parameter is zero, it will return the data with the current
  // version in the database. ValueInfo contains the version and its version.
  // Return nullptr if there is an error.
  std::unique_ptr<ValueInfo> Get(const std::string& key, int version);

  // Obtain the latest values of the keys within [min_key, max_key].
  // Keys should be comparable.
  std::unique_ptr<Items> GetKeyRange(const std::string& min_key,
                                     const std::string& max_key);

  // Obtain the histories of `key` with the versions in [min_version,
  // max_version]
  std::unique_ptr<Items> GetKeyHistory(const std::string& key, int min_version,
                                       int max_version);

  // Obtain the top `top_number` histories of the `key`.
  std::unique_ptr<Items> GetKeyTopHistory(const std::string& key,
                                          int top_number);

  // Non-version-based Interfaces.
  // These interfaces are not compatible with the version-based interfaces
  // above.
  int Set(const std::string& key, const std::string& data);
  std::unique_ptr<std::string> Get(const std::string& key);
  std::unique_ptr<std::string> GetRange(const std::string& min_key,
                                        const std::string& max_key);

  // Execute an arbitrary SQL query with the ReSQL query service, based on DuckDB.
  std::unique_ptr<std::string> QueryResQL(const std::string& sql_query);

  // Secondary indexes (issue #180). The server builds the stored key from
  // these parts. Return 0 on success, -3 if the servers rejected the request,
  // or the send error from TransactionConstructor::SendRequest (-1, -2).
  int CreateIndexEntry(const std::string& index_name,
                       const std::vector<std::string>& attributes,
                       const std::string& primary_key);
  int DeleteIndexEntry(const std::string& index_name,
                       const std::vector<std::string>& attributes,
                       const std::string& primary_key);
  int UpdateIndexEntry(const std::string& index_name,
                       const std::vector<std::string>& old_attributes,
                       const std::vector<std::string>& new_attributes,
                       const std::string& primary_key);
  // Primary keys whose entry starts with `attribute_prefix`, each once and
  // sorted. Empty prefix means the whole index. nullptr if the send failed.
  std::unique_ptr<Items> QueryByIndex(
      const std::string& index_name,
      const std::vector<std::string>& attribute_prefix);
};

}  // namespace resdb
