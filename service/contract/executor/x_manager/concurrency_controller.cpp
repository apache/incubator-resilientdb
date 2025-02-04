#include "service/contract/executor/x_manager/concurrency_controller.h"

#include <glog/logging.h>

namespace resdb {
namespace contract {
namespace x_manager {

ConcurrencyController::ConcurrencyController(DataStorage* storage)
    : storage_(storage) {}

const DataStorage* ConcurrencyController::GetStorage() const {
  return storage_;
}

}  // namespace x_manager
}  // namespace contract
}  // namespace resdb
