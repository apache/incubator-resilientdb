#include "client/resdb_txn_client.h"

#include <glog/logging.h>

#include <future>
#include <thread>

namespace resdb {

ResDBTxnClient::ResDBTxnClient(const ResDBConfig& config)
    : config_(config),
      replicas_(config.GetReplicaInfos()),
      recv_timeout_(1) /*1s*/ {}

std::unique_ptr<ResDBClient> ResDBTxnClient::GetResDBClient(
    const std::string& ip, int port) {
  return std::make_unique<ResDBClient>(ip, port);
}

// Obtain ReplicaState of each replica.
absl::StatusOr<std::vector<std::pair<uint64_t, std::string>>>
ResDBTxnClient::GetTxn(uint64_t min_seq, uint64_t max_seq) {
  QueryRequest request;
  request.set_min_seq(min_seq);
  request.set_max_seq(max_seq);

  std::vector<std::unique_ptr<ResDBClient>> clients;
  std::vector<std::thread> ths;
  std::string final_str;
  std::mutex mtx;
  std::condition_variable resp_cv;
  bool success = false;
  std::map<std::string, int> recv_count;
  for (const auto& replica : replicas_) {
    std::unique_ptr<ResDBClient> client =
        GetResDBClient(replica.ip(), replica.port());
    ResDBClient* client_ptr = client.get();
    clients.push_back(std::move(client));

    ths.push_back(std::thread(
        [&](ResDBClient* client) {
          std::string response_str;
          int ret = client->SendRequest(request, Request::TYPE_QUERY);
          if (ret) {
            return;
          }
          client->SetRecvTimeout(1000);
          ret = client->RecvRawMessageStr(&response_str);
          if (ret == 0) {
            std::unique_lock<std::mutex> lck(mtx);
            recv_count[response_str]++;
            // receive f+1 count.
            if (recv_count[response_str] == config_.GetMinClientReceiveNum()) {
              final_str = response_str;
              success = true;
              // notify the main thread.
              resp_cv.notify_all();
            }
          }
          return;
        },
        client_ptr));
  }

  {
    std::unique_lock<std::mutex> lck(mtx);
    resp_cv.wait_for(lck, std::chrono::seconds(recv_timeout_));
    // Time out or done, close all the client.
    for (auto& client : clients) {
      client->Close();
    }
  }

  // wait for all theads done.
  for (auto& th : ths) {
    if (th.joinable()) {
      th.join();
    }
  }

  std::vector<std::pair<uint64_t, std::string>> txn_resp;
  QueryResponse resp;
  if (success && final_str.empty()) {
    return txn_resp;
  }

  if (final_str.empty() || !resp.ParseFromString(final_str)) {
    LOG(ERROR) << "parse fail len:" << final_str.size();
    return absl::InternalError("recv data fail.");
  }
  for (auto& transaction : resp.transactions()) {
    txn_resp.push_back(std::make_pair(transaction.seq(), transaction.data()));
  }
  return txn_resp;
}

}  // namespace resdb
