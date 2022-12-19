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

#include "application/kv_server_executor/kv_server_executor.h"

#include <glog/logging.h>

#include "proto/kv_server.pb.h"

namespace resdb {

KVServerExecutor::KVServerExecutor(const ResConfigData& config_data,
                                   char* cert_file)
    : l_storage_layer_(cert_file, config_data),
      r_storage_layer_(cert_file, config_data) {
  equip_rocksdb_ = config_data.rocksdb_info().enable_rocksdb();
  equip_leveldb_ = config_data.leveldb_info().enable_leveldb();
}

KVServerExecutor::KVServerExecutor(void) {}

std::unique_ptr<std::string> KVServerExecutor::ExecuteData(
    const std::string& request) {
  KVRequest kv_request;
  KVResponse kv_response;

  if (!kv_request.ParseFromString(request)) {
    LOG(ERROR) << "parse data fail";
    return nullptr;
  }

  if (kv_request.cmd() == KVRequest::SET) {
    Set(kv_request.key(), kv_request.value());
  } else {
    kv_response.set_value(Get(kv_request.key()));
  }

  std::unique_ptr<std::string> resp_str = std::make_unique<std::string>();
  if (!kv_response.SerializeToString(resp_str.get())) {
    return nullptr;
  }

  return resp_str;
}

void KVServerExecutor::Set(const std::string& key, const std::string& value) {
  if (equip_rocksdb_) {
    r_storage_layer_.setDurable(key, value);
  } else if (equip_leveldb_) {
    l_storage_layer_.setDurable(key, value);
  } else {
    kv_map_[key] = value;
  }
}

std::string KVServerExecutor::Get(const std::string& key) {
  if (equip_rocksdb_)
    return r_storage_layer_.getDurable(key);
  else if (equip_leveldb_)
    return l_storage_layer_.getDurable(key);
  else
    return kv_map_[key];
}
}  // namespace resdb
