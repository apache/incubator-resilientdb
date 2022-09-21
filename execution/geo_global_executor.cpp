#include "execution/geo_global_executor.h"

#include <glog/logging.h>

namespace resdb {

GeoGlobalExecutor::GeoGlobalExecutor(
    std::unique_ptr<TransactionExecutorImpl> geo_executor_impl)
    : geo_executor_impl_(std::move(geo_executor_impl)) {}

int GeoGlobalExecutor::Execute(std::unique_ptr<Request> request) {
  BatchClientRequest batch_request;
  if (!batch_request.ParseFromString(request->data())) {
    LOG(ERROR) << "[GeoGlobalExecutor] parse data fail!";
    return -1;
  }
  if (request->data().empty()) {
    LOG(ERROR) << "[GeoGlobalExecutor] request->data() is empty ";
    return 1;
  }
  geo_executor_impl_->ExecuteBatch(batch_request);
  return 0;
}

}  // namespace resdb
