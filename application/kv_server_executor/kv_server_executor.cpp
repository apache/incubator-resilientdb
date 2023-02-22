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
#include <pybind11/embed.h>

#include "proto/kv_server.pb.h"

namespace py = pybind11;
using namespace py::literals;

namespace resdb {

KVServerExecutor::KVServerExecutor(const ResConfigData& config_data,
                                   char* cert_file)
    : l_storage_layer_(cert_file, config_data),
      r_storage_layer_(cert_file, config_data) {
  equip_rocksdb_ = config_data.rocksdb_info().enable_rocksdb();
  equip_leveldb_ = config_data.leveldb_info().enable_leveldb();
  require_txn_validation_ = config_data.require_txn_validation();
  if (require_txn_validation_) {
    py::initialize_interpreter();
    py::exec(R"(
      import json
    )");
  }
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
  } else if (kv_request.cmd() == KVRequest::GET) {
    kv_response.set_value(Get(kv_request.key()));
  } else if (kv_request.cmd() == KVRequest::GETVALUES) {
    kv_response.set_value(GetValues());
  } else if (kv_request.cmd() == KVRequest::GETRANGE) {
    kv_response.set_value(GetRange(kv_request.key(), kv_request.value()));
  }

  std::unique_ptr<std::string> resp_str = std::make_unique<std::string>();
  if (!kv_response.SerializeToString(resp_str.get())) {
    return nullptr;
  }

  return resp_str;
}

// Validate transactions committed by the Python SDK
bool KVServerExecutor::Validate(const std::string& transaction) {
  auto locals = py::dict("transaction"_a = transaction);

  py::exec(R"(
    from sdk_validator.validator import is_valid_tx

    try:
      txn_dict = json.loads(transaction)
      ret = is_valid_tx(txn_dict)
      is_valid = ret[0] == 0
    except (KeyError, AttributeError, ValueError):
      is_valid = False
  )",
           py::globals(), locals);

  bool is_valid = locals["is_valid"].cast<bool>();

  return is_valid;
}

void KVServerExecutor::Set(const std::string& key, const std::string& value) {
  if (require_txn_validation_) {
    bool is_valid = Validate(value);
    if (!is_valid) {
      LOG(ERROR) << "Invalid transaction for " << key;
      return;
    }
  }

  if (equip_rocksdb_) {
    r_storage_layer_.SetValue(key, value);
  } else if (equip_leveldb_) {
    l_storage_layer_.SetValue(key, value);
  } else {
    kv_map_[key] = value;
  }
}

std::string KVServerExecutor::Get(const std::string& key) {
  if (equip_rocksdb_) {
    return r_storage_layer_.GetValue(key);
  } else if (equip_leveldb_) {
    return l_storage_layer_.GetValue(key);
  } else {
    auto search = kv_map_.find(key);
    if (search != kv_map_.end())
      return search->second;
    else
      return "";
  }
}

std::string KVServerExecutor::GetValues() {
  if (equip_rocksdb_) {
    return r_storage_layer_.GetAllValues();
  } else if (equip_leveldb_) {
    return l_storage_layer_.GetAllValues();
  } else {
    std::string values = "[";
    bool first_iteration = true;
    for (auto kv : kv_map_) {
      if (!first_iteration) values.append(",");
      first_iteration = false;
      values.append(kv.second);
    }
    values.append("]");
    return values;
  }
}

// Get values on a range of keys
std::string KVServerExecutor::GetRange(const std::string& min_key,
                                       const std::string& max_key) {
  if (equip_rocksdb_) {
    return r_storage_layer_.GetRange(min_key, max_key);
  } else if (equip_leveldb_) {
    return l_storage_layer_.GetRange(min_key, max_key);
  } else {
    std::string values = "[";
    bool first_iteration = true;
    for (auto kv : kv_map_) {
      if (kv.first >= min_key && kv.first <= max_key) {
        if (!first_iteration) values.append(",");
        first_iteration = false;
        values.append(kv.second);
      }
    }
    values.append("]");
    return values;
  }
}
}  // namespace resdb
