#include "platform/consensus/ordering/bullshark/framework/consensus.h"

#include <glog/logging.h>
#include <unistd.h>

#include "common/utils/utils.h"

namespace resdb {
namespace bullshark {

BullSharkConsensus::BullSharkConsensus(const ResDBConfig& config,
                             std::unique_ptr<TransactionManager> executor)
    : Consensus(config, std::move(executor)) {
  int total_replicas = config_.GetReplicaNum();
  int f = (total_replicas - 1) / 3;

  Init();

  if (config_.GetPublicKeyCertificateInfo()
          .public_key()
          .public_key_info()
          .type() != CertificateKeyInfo::CLIENT) {
    bullshark_ = std::make_unique<BullShark>(config_.GetSelfInfo().id(), f,
                                   total_replicas, GetSignatureVerifier(), config_);

    bullshark_->SetSingleCallFunc(
        [&](int type, const google::protobuf::Message& msg, int node_id) {
          return SendMsg(type, msg, node_id);
        });

    bullshark_->SetBroadcastCallFunc(
        [&](int type, const google::protobuf::Message& msg) {
          return Broadcast(type, msg);
        });

    bullshark_->SetCommitFunc([&](const google::protobuf::Message& msg) {
      return CommitMsg(dynamic_cast<const Transaction&>(msg));
    });
  }
  LOG(ERROR)<<"BullShark init consensus done";
}

int BullSharkConsensus::ProcessCustomConsensus(std::unique_ptr<Request> request) {
  if (request->user_type() == MessageType::NewBlock) {
    std::unique_ptr<Proposal> p = std::make_unique<Proposal>();
    if (!p->ParseFromString(request->data())) {
      LOG(ERROR) << "parse proposal fail";
      assert(1 == 0);
      return -1;
    }
    bullshark_->ReceiveBlock(std::move(p));
  } else if (request->user_type() == MessageType::BlockACK) {
    std::unique_ptr<Metadata> metadata = std::make_unique<Metadata>();
    if (!metadata->ParseFromString(request->data())) {
      LOG(ERROR) << "parse proposal fail";
      assert(1 == 0);
      return -1;
    }
    bullshark_->ReceiveBlockACK(std::move(metadata));
  } else if (request->user_type() == MessageType::Cert) {
    std::unique_ptr<Certificate> cert = std::make_unique<Certificate>();
    if (!cert->ParseFromString(request->data())) {
      LOG(ERROR) << "parse proposal fail";
      assert(1 == 0);
      return -1;
    }
    bullshark_->ReceiveBlockCert(std::move(cert));
  }
  return 0;
}

int BullSharkConsensus::ProcessNewTransaction(std::unique_ptr<Request> request) {
  std::unique_ptr<Transaction> txn = std::make_unique<Transaction>();
  txn->set_data(request->data());
  txn->set_hash(request->hash());
  txn->set_proxy_id(request->proxy_id());
  return bullshark_->ReceiveTransaction(std::move(txn));
}

int BullSharkConsensus::CommitMsg(const google::protobuf::Message& msg) {
  return CommitMsgInternal(dynamic_cast<const Transaction&>(msg));
}

int BullSharkConsensus::CommitMsgInternal(const Transaction& txn) {
  std::unique_ptr<Request> request = std::make_unique<Request>();
  request->set_queuing_time(txn.queuing_time());
  request->set_data(txn.data());
  request->set_seq(txn.id());
  request->set_proxy_id(txn.proxy_id());
  transaction_executor_->AddExecuteMessage(std::move(request));
  return 0;
}

}  // namespace bullshark
}  // namespace resdb
