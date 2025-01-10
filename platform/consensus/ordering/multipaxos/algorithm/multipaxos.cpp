#include "platform/consensus/ordering/multipaxos/algorithm/multipaxos.h"

#include <glog/logging.h>
#include "common/utils/utils.h"


namespace resdb {
namespace multipaxos {

MultiPaxos::MultiPaxos(int id, int f, int total_num, SignatureVerifier * verifier)
  : ProtocolBase(id, f, total_num){

    LOG(ERROR)<<"id:"<<id<<" f:"<<f<<" total:"<<total_num_;

    proposal_manager_ = std::make_unique<ProposalManager>(id, 2*f_+1, verifier);
    send_thread_ = std::thread(&MultiPaxos::AsyncSend, this);
    commit_thread_ = std::thread(&MultiPaxos::AsyncCommit, this);
    commit_seq_thread_ = std::thread(&MultiPaxos::AsyncCommitSeq, this);
    learn_thread_ = std::thread(&MultiPaxos::AsyncLearn, this);
    batch_size_ = 15;
    start_seq_ = 1;
    master_ = 1;
}

MultiPaxos::~MultiPaxos() {
}


void MultiPaxos::AsyncSend() {
  while (!IsStop()) {
    auto txn = txns_.Pop();
    if(txn == nullptr){
      continue;
    }

    std::vector<std::unique_ptr<Transaction> > txns;
    txns.push_back(std::move(txn));
    for(int i = 1; i < batch_size_; ++i){
      auto txn = txns_.Pop();
      if(txn == nullptr){
        break;
      }
      txns.push_back(std::move(txn));
    }

    std::unique_ptr<Proposal> proposal =  nullptr;
    {
      proposal = proposal_manager_ -> GenerateProposal(txns);
      //LOG(ERROR)<<"propose view:"<<proposal->header().view();
    }
    broadcast_call_(MessageType::Propose, *proposal);
  }
}


void MultiPaxos::AddCommitData(std::unique_ptr<Proposal> p){
  std::unique_lock<std::mutex> lk(n_mutex_);
  commit_data_[p->header().proposal_id()] = std::move(p);
  vote_cv_.notify_one();
}

std::unique_ptr<Proposal> MultiPaxos::GetCommitData(){
  std::unique_lock<std::mutex> lk(n_mutex_);
  auto p = std::move(commit_data_.begin()->second);
  commit_data_.erase(commit_data_.begin());
  start_seq_++;
  return p;
}

bool MultiPaxos::Wait() {
  auto Ready = [&]() {
    return !commit_data_.empty() && commit_data_.begin()->first == start_seq_;
  };

  while(!IsStop()){
      std::unique_lock<std::mutex> lk(n_mutex_);
      vote_cv_.wait_for(lk, std::chrono::microseconds(1000),
          [&] { return Ready(); });
      if(Ready()){
        return true;
      }
    }
    return false;
}

void MultiPaxos::AsyncCommit() {
  while (!IsStop()) {
    auto p = commit_q_.Pop();
    if(p == nullptr){
      continue;
    }

    int proposer = p->header().proposer();
    int round = p->header().view();
    std::unique_ptr<Proposal> data = nullptr;
    while(data == nullptr) {
      std::unique_lock<std::mutex> lk(mutex_);
      auto it = receive_.find(round);
      if(it == receive_.end()){
        //LOG(ERROR)<<" proposer:"<<proposer<<" round:"<<round<<" not exist";
        usleep(100);
        continue;
      }
      assert(it != receive_.end());
      auto dit = it->second.find(proposer);
      if(dit == it->second.end()){
        continue;
      }
      assert(dit != it->second.end());
      data = std::move(dit->second);
    }
    assert(data != nullptr);
    AddCommitData(std::move(data));
  }
}

void MultiPaxos::AsyncCommitSeq() {
  int seq = 1;
  while (!IsStop()) {
    if(!Wait()){
      break;
    }
    std::unique_ptr<Proposal> data = GetCommitData();
    //int proposer = data->header().proposer();
    //int round = data->header().view();
    //int proposal_seq = data->header().proposal_id();
    //LOG(ERROR)<<" commit proposer:"<<proposer<< " round:"<<round<<" seq:"<<proposal_seq;
    for(Transaction& txn : *data->mutable_transactions()){
      txn.set_id(seq++);
      Commit(txn);
    }
  }
}

void MultiPaxos::AsyncLearn() {
  int seq = 1;
  while (!IsStop()) {
    auto proposal = learn_q_.Pop();
    if(proposal == nullptr){
      continue;
    }

    int round = proposal->header().view();
    int sender = proposal->sender();
    int proposer = proposal->header().proposer();

    std::unique_lock<std::mutex> lk(learn_mutex_);
    learn_receive_[round].insert(sender);
    //LOG(ERROR)<<"RECEIVE proposer learn:"<<round<<" size:"<<learn_receive_[round].size()<<" proposer:"<<proposer<<" sender:"<<sender;
    if(learn_receive_[round].size() == f_+1){
      CommitProposal(std::move(proposal));
    }

  }
}


bool MultiPaxos::ReceiveTransaction(std::unique_ptr<Transaction> txn) {
  if(id_ != master_) {
    SendMessage(MessageType::Redirect, *txn, master_);
    return true;
  }
  txn->set_proposer(id_);
  txns_.Push(std::move(txn));
  return true;
}

bool MultiPaxos::ReceiveProposal(std::unique_ptr<Proposal> proposal) {
  int round = proposal->header().view();
  int proposer = proposal->sender();
  int seq = proposal->header().proposal_id();

  bool done = false;
  {
    std::unique_lock<std::mutex> lk(mutex_);
    //LOG(ERROR)<<"recv proposal from:"<<proposer<<" round:"<<round<<" seq:"<<seq;
    receive_[round][proposer] = std::move(proposal);
    done = true;
  }
  if(done) {
    Proposal learn_proposal;
    learn_proposal.mutable_header()->set_view(round);
    learn_proposal.mutable_header()->set_proposer(proposer);
    learn_proposal.mutable_header()->set_proposal_id(seq);
    learn_proposal.set_sender(id_);
    Broadcast(MessageType::Learn, learn_proposal);
  }
  return true;
}

bool MultiPaxos::ReceiveLearn(std::unique_ptr<Proposal> proposal) {
  learn_q_.Push(std::move(proposal));
  return true;
}

void MultiPaxos::CommitProposal(std::unique_ptr<Proposal> p){
  commit_q_.Push(std::move(p));
}


}  // namespace tusk
}  // namespace resdb
