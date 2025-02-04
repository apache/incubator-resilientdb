#pragma once

#include <memory>
#include <string>
#include <vector>

#include "service/contract/benchmark/generator/generator.h"

namespace resdb {

class StringGenerator {
 public:
  StringGenerator(std::unique_ptr<Generator> generator);

  void AddString(const std::string& addr);

  std::string Next();

 private:
  std::unique_ptr<Generator> generator_;
  std::vector<std::string> address_;
  int total_;
};

}  // namespace resdb
