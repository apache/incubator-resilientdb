#include "kv_server/kv_server_executor.h"

#include <glog/logging.h>

#include "proto/kv_server.pb.h"

namespace resdb {

KVServerExecutor::KVServerExecutor(const ResConfigData& config_data,
                                   char* cert_file)
#ifdef EnableStorage
    : l_storage_layer_(cert_file, config_data),
      r_storage_layer_(cert_file, config_data) {
  equip_rocksdb_ = config_data.rocksdb_info().enable_rocksdb();
  equip_leveldb_ = config_data.leveldb_info().enable_leveldb();
}
#else 
{
}
#endif

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
#ifdef EnableStorage
  if (equip_rocksdb_) {
    r_storage_layer_.setDurable(key, value);
  } else if (equip_leveldb_) {
    l_storage_layer_.setDurable(key, value);
  } else {
    kv_map_[key] = value;
  }
#else
    kv_map_[key] = value;
#endif
}

std::string KVServerExecutor::Get(const std::string& key) {
#ifdef EnableStorage
  if (equip_rocksdb_)
    return r_storage_layer_.getDurable(key);
  else if (equip_leveldb_)
    return l_storage_layer_.getDurable(key);
  else
    return kv_map_[key];
#else
    return kv_map_[key];
#endif
}
}  // namespace resdb
