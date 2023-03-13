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

#include "client/resdb_txn_client.h"
#include "config/resdb_config_utils.h"
#include "crow.h"
#include "kv_client/resdb_kv_client.h"
#include "proto/kv_server.pb.h"
#include "proto/signature_info.pb.h"
#include "sdk_client/sdk_transaction.h"

namespace resdb {

/**
 *
 * Creates a HTTP service using [Crow Cpp](https://crowcpp.org/master/). The
 * Crow app runs on http://localhost:18000 and defines API endpoints for a
 * direct connection to ResillientDB.
 *
 * The following endpoints are defined:
 * Method | Route                              | Description
 * ------ | -----------------------------------| -------------
 * GET    | /v1/transactions                   | Gets all values
 * GET    | /v1/transactions/<string>          | Gets value of specific id
 * GET    | /v1/transactions/<string>/<string> | Gets values based on key range
 * POST   | /v1/transactions/commit            | Set a key-value pair
 * GET    | /v1/blocks/<int>                   | Retrieve blocks in batches of
 * size of the int parameter GET    | /v1/blocks/<int>/<int>             |
 * Retrieve blocks within a range
 *
 */

class CrowService {
 public:
  /** Initializes Crow Service
   *
   * @param config configuration file of the client for initializing the KV
   * Client
   * @param server_config configuration file of a replica for initializing the
   * TXN Client
   * @param port_num port number
   */
  CrowService(ResDBConfig config, ResDBConfig server_config,
              uint16_t port_num = 18000);
  /** Runs the Crow app and exposes the endpoints
   */
  void run();

 private:
  /* Helper function used in the anonymous functions for reading
   * from blocks. It parses the request, then based on the command
   * type, it returns a relevant JSON string.
   */
  std::string ParseKVRequest(const KVRequest &kv_request);
  ResDBConfig config_;
  ResDBConfig server_config_;
  uint16_t port_num_;
  ResDBKVClient kv_client_;
  ResDBTxnClient txn_client_;
};

}  // namespace resdb
