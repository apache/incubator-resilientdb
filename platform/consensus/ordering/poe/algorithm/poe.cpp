#include "platform/consensus/ordering/poe/algorithm/poe.h"

#include <glog/logging.h>

#include "common/crypto/signature_verifier.h"
#include "common/utils/utils.h"

namespace resdb {
namespace poe {

PoE::PoE(int id, int f, int total_num, SignatureVerifier* verifier)
    : ProtocolBase(id, f, total_num), verifier_(verifier) {

  LOG(ERROR) << "get proposal graph";
  id_ = id;
  total_num_ = total_num;
  f_ = f;
  is_stop_ = false;
  seq_ = 0;
}

PoE::~PoE() {
  is_stop_ = true;
}

bool PoE::IsStop() { return is_stop_; }

bool PoE::ReceiveTransaction(std::unique_ptr<Transaction> txn) {
  // LOG(ERROR)<<"recv txn:";
  txn->set_create_time(GetCurrentTime());
  txn->set_seq(seq_++);
  txn->set_proposer(id_);

  Broadcast(MessageType::Propose, *txn);
  return true;
}

bool PoE::ReceivePropose(std::unique_ptr<Transaction> txn) {
  std::string hash = txn->hash();
  int64_t seq = txn->seq();
  int proposer = txn->proposer();
  {
    // LOG(ERROR)<<"recv proposal";
    //LOG(ERROR)<<"recv txn from:"<<txn->proposer()<<" id:"<<txn->seq();
    std::unique_lock<std::mutex> lk(mutex_[seq%1000]);
    data_[seq%1000][txn->hash()]=std::move(txn);
  }

  Proposal proposal;
  proposal.set_hash(hash);
  proposal.set_seq(seq);
  proposal.set_proposer(id_);

  Broadcast(MessageType::Prepare, proposal);
  //LOG(ERROR)<<"receive proposal done";
  return true;
}

bool PoE::ReceivePrepare(std::unique_ptr<Proposal> proposal) {
  std::string hash = proposal->hash();
  int64_t seq = proposal->seq();
  int proposer = proposal->proposer();
  std::unique_ptr<Transaction> txn = nullptr;
  {
    //LOG(ERROR)<<"recv proposal from:"<<proposal->proposer()<<" id:"<<proposal->seq();
    std::unique_lock<std::mutex> lk(mutex_[seq%1000]);
    received_[seq%1000][seq].insert(proposal->proposer());
    //received_[proposal->hash()].insert(proposal->proposer());
    if(received_[seq%1000][seq].size()==2*f_+1){
      //if(received_[proposal->hash()].size()>=2*f_+1){
      auto it = data_[seq%1000].find(proposal->hash());
      if(it != data_[seq%1000].end()){
        txn = std::move(it->second);
        data_[seq%1000].erase(it);
      }
    }
 //     }
  }
  if(txn != nullptr){
    commit_(*txn);
  std::unique_lock<std::mutex> lk(commit_mutex_);
    committed_[hash] = std::move(txn);

    Proposal proposal;
    proposal.set_hash(hash);
    proposal.set_seq(seq);
    proposal.set_proposer(id_);
    Broadcast(MessageType::Commit, proposal);
  //LOG(ERROR)<<"receive proposal done:"<<seq;
  }
  return true;
}

bool PoE::ReceiveCommit(std::unique_ptr<Proposal> proposal) {
  std::string hash = proposal->hash();
  int64_t seq = proposal->seq();
  int proposer = proposal->proposer();
  std::unique_lock<std::mutex> lk(commit_mutex_);
  commit_received_[seq].insert(proposer);
  //LOG(ERROR)<<"receive commit:"<<proposer<<" num:"<<commit_received_[hash].size()<<" seq:"<<seq;
  if(commit_received_[seq].size()==2*f_+1){
    //LOG(ERROR)<<" commit:";
    if(committed_.find(hash) == committed_.end()){
      // need to re-propose
      // assert(1==0);
    }
    else {
      //assert(committed_.find(hash) != committed_.end());
      committed_.erase(committed_.find(hash));
    }
  }
  return true;

}

}  // namespace poe
}  // namespace resdb
