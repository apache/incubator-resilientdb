#include <glog/logging.h>
#include <unistd.h>

#include <cstdint>
#include <nlohmann/json.hpp>

#include "chain/storage/leveldb.h"
#include "chain/storage/proto/kv.pb.h"
#include "leveldb/options.h"

namespace resdb {
namespace storage {

class MultiSecIndex {
 public:
  MultiSecIndex() = default;
  virtual ~MultiSecIndex() = default;

  // put enconded composite key into db
  // use ResLevelDB::SetValue
  int createCompositeKey(const std::string& primKey,
                         const nlohmann::json& docValue,
                         const std::string& fieldName,
                         const std::string& fieldType);
  // first get field value then convert it into string
  std::string encodeFieldVal(const nlohmann::json& docValue,
                             const std::string& fieldName,
                             const std::string& fieldType);
  // fieldName + fieldVal + primKey
  std::string encodeCompositeKey(std::string& fieldName, std::string& fieldVal,
                                 std::string& primKey);

  // using primary keys got by primKeysbyPrefix to get key value pairs in db
  // assume user already convert field value into string
  int GetByCompositeKey(const std::string& queryFieldName,
                        const std::string& queryFieldVal,
                        std::vector<nlohmann::json>& matched_records);
  // encode into prefix of composite key
  std::string encodePrefix(const std::string& queryFieldName,
                           const std::string& queryFieldVal);
  // get primary keys by single field pair prefix
  int compKeysbyPrefix(std::string& prefix, std::vector<std::string>& compKeys);
  // string subtraction from composite keys to get rid of prefix and get primary
  // keys
  std::vector<std::string> primKeybyCompKey(std::vector<std::string>& compKeys);
  // calling encodeFieldVal to check if the query result's field values is the
  // same as query's.
  bool validateUpdated(std::vector<nlohmann::json>& matched_records,
                       const std::string& queryFieldVal);
};
}  // namespace storage
}  // namespace resdb