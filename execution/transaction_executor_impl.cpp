#include "execution/transaction_executor_impl.h"
namespace resdb {

std::unique_ptr<std::string> TransactionExecutorImpl::ExecuteData(
    const std::string& request) {
  return std::make_unique<std::string>();
}

std::unique_ptr<BatchClientResponse> TransactionExecutorImpl::ExecuteBatch(
    const BatchClientRequest& request) {
  std::unique_ptr<BatchClientResponse> batch_response =
      std::make_unique<BatchClientResponse>();

  for (auto& sub_request : request.client_requests()) {
    std::unique_ptr<std::string> response =
        ExecuteData(sub_request.request().data());
    if (response == nullptr) {
      response = std::make_unique<std::string>();
    }
    batch_response->add_response()->swap(*response);
  }

  return batch_response;
}

}  // namespace resdb
