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

#include <crow.h>

#include "service/kv_service/resdb_kv_client.h"
#include "common/proto/signature_info.pb.h"
#include "platform/config/resdb_config_utils.h"
#include "interface/common/resdb_state_accessor.h"
#include "interface/common/resdb_txn_accessor.h"
#include "service/http_server/sdk_transaction.h"
#include "service/kv_service/proto/kv_server.pb.h"

namespace sdk {

class CrowService {
 public:
  CrowService(resdb::ResDBConfig config, resdb::ResDBConfig server_config,
              uint16_t port_num = 18000);
  void run();

 private:
  std::string GetAllBlocks(int batch_size);
  std::string ParseKVRequest(const KVRequest &kv_request);
  std::string ParseCreateTime(uint64_t createtime);
  resdb::ResDBConfig config_;
  resdb::ResDBConfig server_config_;
  uint16_t port_num_;
  ResDBKVClient kv_client_;
  resdb::ResDBTxnAccessor txn_client_;
  std::unordered_set<crow::websocket::connection*> users;
};

}  // namespace resdb
