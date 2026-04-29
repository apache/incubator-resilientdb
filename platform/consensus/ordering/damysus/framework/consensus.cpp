#include "platform/consensus/ordering/damysus/framework/consensus.h"

#include <glog/logging.h>
#include <unistd.h>
#include "common/utils/utils.h"

namespace resdb {
namespace damysus {

DamysusConsensus::DamysusConsensus(const ResDBConfig& config,
                                   std::unique_ptr<TransactionManager> executor,
                                   oe_enclave_t* enclave)
    : Consensus(config, std::move(executor)) {
  int total_replicas = config_.GetReplicaNum();
  // Damysus uses n=2f+1, so f = (n-1)/2
  int f = (total_replicas - 1) / 2;

  Init();

  if (config_.GetPublicKeyCertificateInfo()
          .public_key()
          .public_key_info()
          .type() != CertificateKeyInfo::CLIENT) {
    damysus_ = std::make_unique<Damysus>(config_.GetSelfInfo().id(), f,
                                         total_replicas, GetSignatureVerifier(),
                                         config, enclave);

    damysus_->SetSingleCallFunc(
        [&](int type, const google::protobuf::Message& msg, int node_id) {
          return SendMsg(type, msg, node_id);
        });

    damysus_->SetBroadcastCallFunc(
        [&](int type, const google::protobuf::Message& msg) {
          return Broadcast(type, msg);
        });

    damysus_->SetCommitFunc([&](const google::protobuf::Message& msg) {
      return CommitMsg(dynamic_cast<const Transaction&>(msg));
    });
  }
  LOG(ERROR) << "Damysus consensus init done";
}

int DamysusConsensus::ProcessCustomConsensus(std::unique_ptr<Request> request) {
  if (request->user_type() == MessageType::NewView) {
    auto msg = std::make_unique<NewViewMsg>();
    if (!msg->ParseFromString(request->data())) {
      LOG(ERROR) << "parse NewViewMsg fail";
      return -1;
    }
    damysus_->ReceiveNewView(std::move(msg));
  } else if (request->user_type() == MessageType::Prepare) {
    auto proposal = std::make_unique<Proposal>();
    if (!proposal->ParseFromString(request->data())) {
      LOG(ERROR) << "parse Proposal fail";
      return -1;
    }
    damysus_->ReceiveProposal(std::move(proposal));
  } else if (request->user_type() == MessageType::PreCommit) {
    auto vote = std::make_unique<PreCommitVote>();
    if (!vote->ParseFromString(request->data())) {
      LOG(ERROR) << "parse PreCommitVote fail";
      return -1;
    }
    damysus_->ReceivePreCommitVote(std::move(vote));
  } else if (request->user_type() == MessageType::Decide) {
    auto decide = std::make_unique<DecideMsg>();
    if (!decide->ParseFromString(request->data())) {
      LOG(ERROR) << "parse DecideMsg fail";
      return -1;
    }
    damysus_->ReceiveDecide(std::move(decide));
  }
  return 0;
}

int DamysusConsensus::ProcessNewTransaction(std::unique_ptr<Request> request) {
  auto txn = std::make_unique<Transaction>();
  txn->set_data(request->data());
  txn->set_hash(request->hash());
  txn->set_proxy_id(request->proxy_id());
  return damysus_->ReceiveTransaction(std::move(txn));
}

int DamysusConsensus::CommitMsg(const google::protobuf::Message& msg) {
  return CommitMsgInternal(dynamic_cast<const Transaction&>(msg));
}

int DamysusConsensus::CommitMsgInternal(const Transaction& txn) {
  std::unique_ptr<Request> request = std::make_unique<Request>();
  request->set_queuing_time(txn.queuing_time());
  request->set_data(txn.data());
  request->set_seq(txn.id());
  request->set_proxy_id(txn.proxy_id());
  transaction_executor_->AddExecuteMessage(std::move(request));
  return 0;
}

}  // namespace damysus
}  // namespace resdb
