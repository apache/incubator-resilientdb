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

#include "interface/rdbc/transaction_constructor.h"
#include "proto/kv/kv.pb.h"

namespace resdb {

// KVClient to send data to the kv server.
class KVClient : public TransactionConstructor {
 public:
  KVClient(const ResDBConfig& config);

  //Version-based interfaces.
  // Obtain the current version before setting a new data
  int Set(const std::string& key, const std::string& data, int version);

  // Obtain the value with a specific version.
  // If the version parameter is zero, it will return the data with the current version in the
  // database. ValueInfo contains the version and its version. 
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
  std::unique_ptr<std::string> GetAllValues();
  std::unique_ptr<std::string> GetRange(const std::string& min_key,
                                        const std::string& max_key);
};

}  // namespace resdb
