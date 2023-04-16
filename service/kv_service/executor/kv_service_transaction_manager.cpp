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

#include "service/kv_service/executor/kv_service_transaction_manager.h"

#include <glog/logging.h>

#include "service/kv_service/proto/kv_server.pb.h"
#include "storage/in_mem_kv_storage.h"

#ifdef ENABLE_LEVELDB
#include "storage/res_leveldb.h"
#endif

#ifdef ENABLE_ROCKSDB
#include "storage/res_rocksdb.h"
#endif

namespace resdb {

KVServiceTransactionManager::KVServiceTransactionManager(
    const ResConfigData& config_data, char* cert_file) {
#ifdef ENABLE_ROCKSDB
  storage_ = NewResRocksDB(cert_file, config_data);
  LOG(INFO)<<"use rocksdb storage.";
#endif

#ifdef ENABLE_LEVELDB
  storage_ = NewResLevelDB(cert_file, config_data);
  LOG(INFO)<<"use leveldb storage.";
#endif

  if (storage_ == nullptr) {
  LOG(INFO)<<"use kv storage.";
    storage_ = NewInMemKVStorage();
  }
}

KVServiceTransactionManager::KVServiceTransactionManager(
    std::unique_ptr<Storage> storage)
    : storage_(std::move(storage)) {}

std::unique_ptr<std::string> KVServiceTransactionManager::ExecuteData(
    const std::string& request) {
  KVRequest kv_request;
  KVResponse kv_response;

  if (!kv_request.ParseFromString(request)) {
    LOG(ERROR) << "parse data fail";
    return nullptr;
  }

  if (kv_request.cmd() == KVRequest::SET) {
    // add to batch
    Set(kv_request.key(), kv_request.value());
  } else if (kv_request.cmd() == KVRequest::GET) {
    // add to batch
    kv_response.set_value(Get(kv_request.key()));
  } else if (kv_request.cmd() == KVRequest::GETVALUES) {
    // add to batch
    kv_response.set_value(GetValues());
  } else if (kv_request.cmd() == KVRequest::GETRANGE) {
    // add to batch
    kv_response.set_value(GetRange(kv_request.key(), kv_request.value()));
  }

  std::unique_ptr<std::string> resp_str = std::make_unique<std::string>();
  if (!kv_response.SerializeToString(resp_str.get())) {
    return nullptr;
  }

  return resp_str;
}

void KVServiceTransactionManager::Set(const std::string& key,
                                      const std::string& value) {
  storage_->SetValue(key, value);
}

std::string KVServiceTransactionManager::Get(const std::string& key) {
  return storage_->GetValue(key);
}

std::string KVServiceTransactionManager::GetValues() {
  return storage_->GetAllValues();
}

// Get values on a range of keys
std::string KVServiceTransactionManager::GetRange(const std::string& min_key,
                                                  const std::string& max_key) {
  return storage_->GetRange(min_key, max_key);
}

}  // namespace resdb
