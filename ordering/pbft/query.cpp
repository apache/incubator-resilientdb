#include "ordering/pbft/query.h"

#include <unistd.h>

#include "glog/logging.h"

namespace resdb {

Query::Query(const ResDBConfig& config, TransactionManager* transaction_manager)
    : config_(config), transaction_manager_(transaction_manager) {}

Query::~Query() {}

int Query::ProcessGetReplicaState(std::unique_ptr<Context> context,
                                  std::unique_ptr<Request> request) {
  ReplicaState replica_state;
  int ret = transaction_manager_->GetReplicaState(&replica_state);
  if (ret == 0) {
    if (context != nullptr && context->client != nullptr) {
      ret = context->client->SendRawMessage(replica_state);
      if (ret) {
        LOG(ERROR) << "send resp" << replica_state.DebugString()
                   << " fail ret:" << ret;
      }
    }
  }
  return ret;
}

int Query::ProcessQuery(std::unique_ptr<Context> context,
                        std::unique_ptr<Request> request) {
  QueryRequest query;
  if (!query.ParseFromString(request->data())) {
    LOG(ERROR) << "parse data fail";
    return -2;
  }
  // LOG(ERROR) << "request:" << query.DebugString();

  QueryResponse response;

  for (uint64_t i = query.min_seq(); i <= query.max_seq(); ++i) {
    Request* ret_request = transaction_manager_->GetRequest(i);
    if (ret_request == nullptr) {
      break;
    }
    Request* txn = response.add_transactions();
    txn->set_data(ret_request->data());
    txn->set_seq(ret_request->seq());
  }

  if (context != nullptr && context->client != nullptr) {
    // LOG(ERROR) << "send response:" << response.DebugString();
    int ret = context->client->SendRawMessage(response);
    if (ret) {
      LOG(ERROR) << "send resp fail ret:" << ret;
    }
  }
  return 0;
}

}  // namespace resdb
