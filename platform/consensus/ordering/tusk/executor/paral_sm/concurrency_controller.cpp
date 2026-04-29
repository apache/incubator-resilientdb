#include "service/contract/executor/paral_sm/concurrency_controller.h"

#include <glog/logging.h>

namespace resdb {
namespace contract {

ConcurrencyController::ConcurrencyController(DataStorage * storage) : storage_(storage){}

const DataStorage * ConcurrencyController::GetStorage() const {
  return storage_;
}



}
} // namespace eevm
