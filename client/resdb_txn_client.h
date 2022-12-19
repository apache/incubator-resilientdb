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

#include "absl/status/statusor.h"
#include "client/resdb_client.h"
#include "config/resdb_config.h"
#include "proto/replica_info.pb.h"

namespace resdb {

// ResDBTxnClient used to obtain the server state of each replica in ResDB.
// The addresses of each replica are provided from the config.
class ResDBTxnClient {
 public:
  ResDBTxnClient(const ResDBConfig& config);
  virtual ~ResDBTxnClient() = default;

  // Obtain ReplicaState of each replica.
  virtual absl::StatusOr<std::vector<std::pair<uint64_t, std::string>>> GetTxn(
      uint64_t min_seq, uint64_t max_seq);

 protected:
  virtual std::unique_ptr<ResDBClient> GetResDBClient(const std::string& ip,
                                                      int port);

 private:
  ResDBConfig config_;
  std::vector<ReplicaInfo> replicas_;
  int recv_timeout_ = 1;
};

}  // namespace resdb
