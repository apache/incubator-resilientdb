#pragma once

#include <gmock/gmock.h>

#include "server/resdb_service.h"

namespace resdb {

class MockResDBService : public ResDBService {
 public:
  MOCK_METHOD(bool, IsRunning, (), (const override));
  MOCK_METHOD(void, SetRunning, (bool), (override));
  MOCK_METHOD(int, Process,
              (std::unique_ptr<Context>, std::unique_ptr<DataInfo>),
              (override));
};

}  // namespace resdb
