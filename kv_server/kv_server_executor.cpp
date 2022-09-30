#include "kv_server/kv_server_executor.h"

#include <glog/logging.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

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
  } else if (kv_request.cmd() == KVRequest::GET) {
    kv_response.set_value(Get(kv_request.key()));
  } else if (kv_request.cmd() == KVRequest::GETVALUES) {
    kv_response.set_value(GetValues());
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
  if (equip_rocksdb_) {
    return r_storage_layer_.getDurable(key);
  } else if (equip_leveldb_) {
    return l_storage_layer_.getDurable(key);
  } else {
    return kv_map_[key];
  }
}

std::string KVServerExecutor::GetValues() {
  if (equip_rocksdb_) {
    return r_storage_layer_.getAllValues();
  } else if (equip_leveldb_) {
    return l_storage_layer_.getAllValues();
  } else {
    std::string values = "[\n";
    for (auto kv : kv_map_) {
      values.append(kv.second);
    }
    values.append("]\n");
    return values;
  }
}
}  // namespace resdb
