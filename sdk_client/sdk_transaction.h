#pragma once

#include <string>

namespace resdb {

struct SDKTransaction {
  std::string id;
  std::string value;
};

SDKTransaction fromJson(const std::string &json);

}  // namespace resdb
