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

#include <glog/logging.h>

#include "interface/common/resdb_txn_accessor.h"
#include "platform/config/resdb_config_utils.h"
#include "proto/kv/kv.pb.h"

using resdb::BatchUserRequest;
using resdb::GenerateResDBConfig;
using resdb::KVRequest;
using resdb::Request;
using resdb::ResDBConfig;
using resdb::ResDBTxnAccessor;

int main(int argc, char** argv) {
  if (argc < 2) {
    printf("<config path> <min_seq> <max_seq>\n");
    return 0;
  }
  std::string config_file = argv[1];
  uint64_t min_seq = 1;
  uint64_t max_seq = 1;
  if (argc >= 3) {
    min_seq = atoi(argv[2]);
  }
  if (argc >= 4) {
    max_seq = atoi(argv[3]);
  }

  ResDBConfig config = GenerateResDBConfig(config_file);

  ResDBTxnAccessor client(config);
  auto resp = client.GetTxn(min_seq, max_seq);
  if (!resp.ok()) {
    LOG(ERROR) << "get replica state fail";
    exit(1);
  }
  for (auto& txn : *resp) {
    BatchUserRequest request;
    KVRequest kv_request;
    if (request.ParseFromString(txn.second)) {
      for (auto& sub_req : request.user_requests()) {
        kv_request.ParseFromString(sub_req.request().data());
        printf("data {\nseq: %lu\n%s}\n", txn.first,
               kv_request.DebugString().c_str());
      }
    }
  }
}
