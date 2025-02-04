#include "service/contract/benchmark/generator/string_generator.h"

#include <assert.h>

namespace resdb {

StringGenerator::StringGenerator(std::unique_ptr<Generator> generator)
    : generator_(std::move(generator)) {}

void StringGenerator::AddString(const std::string& addr) {
  address_.push_back(addr);
}

std::string StringGenerator::Next() {
  int idx = generator_->Next();
  assert(!address_.empty());
  return address_[idx % address_.size()];
}

}  // namespace resdb
