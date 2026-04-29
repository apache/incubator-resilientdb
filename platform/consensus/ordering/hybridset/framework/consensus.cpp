#include "platform/consensus/ordering/hybridset/framework/consensus.h"

#include <glog/logging.h>
#include <unistd.h>
#include "common/utils/utils.h"

namespace resdb {
namespace hybridset {

std::unique_ptr<HybridSetPerformanceManager> HybridSetConsensus::GetPerformanceManager() {
  return config_.IsPerformanceRunning()
      ? std::make_unique<HybridSetPerformanceManager>(
            config_, GetBroadCastClient(), GetSignatureVerifier())
      : nullptr;
}

HybridSetConsensus::HybridSetConsensus(const ResDBConfig& config,
                                       std::unique_ptr<TransactionManager> executor,
                                       oe_enclave_t* enclave)
    : Consensus(config, std::move(executor)) {
  int total_replicas = config_.GetReplicaNum();
  // HybridSet uses n=2f+1, so f = (n-1)/2
  int f = (total_replicas - 1) / 2;

  // HybridSet is leaderless: ALL nodes need transactions via BroadCast.
  SetPerformanceManager(GetPerformanceManager());

  Init();

  if (config_.GetPublicKeyCertificateInfo()
          .public_key()
          .public_key_info()
          .type() != CertificateKeyInfo::CLIENT) {
    hybridset_ = std::make_unique<HybridSet>(config_.GetSelfInfo().id(), f,
                                             total_replicas, GetSignatureVerifier(),
                                             config, enclave);

    hybridset_->SetSingleCallFunc(
        [&](int type, const google::protobuf::Message& msg, int node_id) {
          return SendMsg(type, msg, node_id);
        });

    hybridset_->SetBroadcastCallFunc(
        [&](int type, const google::protobuf::Message& msg) {
          return Broadcast(type, msg);
        });

    hybridset_->SetCommitFunc([&](const google::protobuf::Message& msg) {
      return CommitMsg(dynamic_cast<const Transaction&>(msg));
    });
  }
  LOG(ERROR) << "HybridSet consensus init done";
}

int HybridSetConsensus::ProcessCustomConsensus(std::unique_ptr<Request> request) {
  if (!hybridset_) {
    return 0;  // Client node — skip consensus messages
  }
  if (request->user_type() == MessageType::Bcast) {
    auto msg = std::make_unique<BcastMsg>();
    if (!msg->ParseFromString(request->data())) {
      LOG(ERROR) << "parse BcastMsg fail";
      return -1;
    }
    hybridset_->ReceiveBcast(std::move(msg));
  } else if (request->user_type() == MessageType::Echo) {
    auto msg = std::make_unique<EchoMsg>();
    if (!msg->ParseFromString(request->data())) {
      LOG(ERROR) << "parse EchoMsg fail";
      return -1;
    }
    hybridset_->ReceiveEcho(std::move(msg));
  } else if (request->user_type() == MessageType::Vote) {
    auto msg = std::make_unique<VoteMsg>();
    if (!msg->ParseFromString(request->data())) {
      LOG(ERROR) << "parse VoteMsg fail";
      return -1;
    }
    hybridset_->ReceiveVote(std::move(msg));
  }
  return 0;
}

int HybridSetConsensus::ProcessNewTransaction(std::unique_ptr<Request> request) {
  if (!hybridset_) {
    return 0;  // Client node — skip
  }
  auto txn = std::make_unique<Transaction>();
  txn->set_data(request->data());
  txn->set_hash(request->hash());
  txn->set_proxy_id(request->proxy_id());
  return hybridset_->ReceiveTransaction(std::move(txn));
}

int HybridSetConsensus::CommitMsg(const google::protobuf::Message& msg) {
  return CommitMsgInternal(dynamic_cast<const Transaction&>(msg));
}

int HybridSetConsensus::CommitMsgInternal(const Transaction& txn) {
  std::unique_ptr<Request> request = std::make_unique<Request>();
  request->set_queuing_time(txn.queuing_time());
  request->set_data(txn.data());
  request->set_seq(txn.id());
  request->set_proxy_id(txn.proxy_id());
  transaction_executor_->AddExecuteMessage(std::move(request));
  return 0;
}

}  // namespace hybridset
}  // namespace resdb
