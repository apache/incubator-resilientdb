#include "sdk_client/sdk_transaction.h"

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace resdb {

SDKTransaction fromJson(const std::string &json) {
  rapidjson::Document doc;
  doc.Parse(json.c_str());
  SDKTransaction transaction{};
  auto getString = [&doc](const char *id) { return doc[id].GetString(); };
  transaction.id = getString("id");
  transaction.value = json;

  return transaction;
}

}  // namespace resdb
