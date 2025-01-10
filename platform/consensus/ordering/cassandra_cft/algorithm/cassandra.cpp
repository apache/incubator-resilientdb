#include "platform/consensus/ordering/cassandra_cft/algorithm/cassandra.h"

#include <glog/logging.h>

#include "common/crypto/signature_verifier.h"
#include "common/utils/utils.h"

#define Fail

namespace resdb {
namespace cassandra_cft {

Cassandra::Cassandra(int id, int f, int total_num, bool failure_mode)
    : ProtocolBase(id, f, total_num), failure_mode_(failure_mode) {

  LOG(ERROR) << "get proposal graph";
  id_ = id;
  total_num_ = total_num;
  f_ = f;
  is_stop_ = false;
  //timeout_ms_ = 10000;
  timeout_ms_ = 60000;
  local_txn_id_ = 1;
  local_proposal_id_ = 1;
  batch_size_ = 15;

  recv_num_ = 0;
  execute_num_ = 0;
  executed_ = 0;
  committed_num_ = 0;
  precommitted_num_ = 0;
  execute_id_ = 1;

  graph_ = std::make_unique<ProposalGraph>(f_);
  proposal_manager_ = std::make_unique<ProposalManager>(id, f, total_num, graph_.get());

  graph_->SetCommitCallBack(
      [&](const Proposal& proposal) { 
      //CommitProposal(proposal); 
    });

  proposal_manager_->SetCommitCallBack(
      [&](std::unique_ptr<Proposal> proposal) { 
      CommitProposal(std::move(proposal));
    });

  //Reset();

  //consensus_thread_ = std::thread(&Cassandra::AsyncConsensus, this);

  block_thread_ = std::thread(&Cassandra::BroadcastTxn, this);
  sbc_thread_ = std::thread(&Cassandra::SBC, this);

  commit_thread_ = std::thread(&Cassandra::AsyncCommit, this);

  global_stats_ = Stats::GetGlobalStats();

  //prepare_thread_ = std::thread(&Cassandra::AsyncPrepare, this);
}

Cassandra::~Cassandra() {
  is_stop_ = true;
  if (consensus_thread_.joinable()) {
    consensus_thread_.join();
  }
  if (commit_thread_.joinable()) {
    commit_thread_.join();
  }
  if (prepare_thread_.joinable()) {
    prepare_thread_.join();
  }
}

void Cassandra::SetPrepareFunction(std::function<int(const Transaction&)> prepare){
  prepare_ = prepare;
}

bool Cassandra::IsStop() { return is_stop_; }

bool Cassandra::ReceiveTransaction(std::unique_ptr<Transaction> txn) {
  // LOG(ERROR)<<"recv txn:";
  txn->set_create_time(GetCurrentTime());
  txns_.Push(std::move(txn));
  recv_num_++;
  return true;
}

void Cassandra::Notify(int round) {
  std::unique_lock<std::mutex> lk(mutex_);
  SetSendNext(round);
  vote_cv_.notify_one();
}

void Cassandra::BroadcastTxn() {
  std::vector<std::unique_ptr<Transaction>> txns;
  while (!IsStop()) {
    std::unique_ptr<Transaction> txn = txns_.Pop();
    if (txn == nullptr) {
      continue;
    }
    txns.push_back(std::move(txn));

    while (!IsStop()) {
      //int current_round = proposal_manager_->CurrentRound();
      std::unique_lock<std::mutex> lk(mutex_);
      vote_cv_.wait_for(lk, std::chrono::microseconds(10000),
                        [&] { return CanSendNext(proposal_manager_->CurrentRound()); });

      if (CanSendNext(proposal_manager_->CurrentRound())) {
        start_ = 1;
        break;
      }
    }

    for(int i = 1; i < batch_size_; ++i){
      std::unique_ptr<Transaction> txn = txns_.Pop(0);
      if(txn == nullptr){
        break;
      }
      txns.push_back(std::move(txn));
    }

    //LOG(ERROR)<<" make block";
    std::unique_ptr<Proposal> proposal = proposal_manager_->MakeBlock(txns);
    //LOG(ERROR)<<" send new proposal :"<<txns.size()<<" height:"<<proposal->header().height();
    Broadcast(MessageType::NewProposal, *proposal);
    txns.clear();
    SetSBCSendReady(proposal->header().height());
    //LOG(ERROR)<<" make block done";
  }
}

void Cassandra::SetSBCSendReady(int round) {
  std::unique_lock<std::mutex> lk(send_ready_mutex_);
  has_sent_ = round;
  send_ready_cv_.notify_one();
  //LOG(ERROR)<<" set send ready round:"<<round;
}

bool Cassandra::SBCSendReady(int round) {
  return has_sent_ == round;
}

void Cassandra::SetSendNext(int round) {
  //LOG(ERROR)<<" set send next:"<<round;
  sent_next_ = round;
}

bool Cassandra::CanSendNext(int round) {
  //LOG(ERROR)<<" can send next:"<<round;
  return sent_next_ == round;
}


bool Cassandra::SBCRecvReady(int round) {
  return proposal_manager_->Ready(round);
}

void Cassandra::NotifyRecvReady() {
    std::unique_lock<std::mutex> lk(recv_ready_mutex_);
    recv_ready_cv_.notify_one();
}

void Cassandra::SBC() {
  std::vector<std::unique_ptr<Transaction>> txns;
  int round = 1;
  while (!IsStop()) {
    while (!IsStop()) {
      //int current_round = proposal_manager_->CurrentRound();
      std::unique_lock<std::mutex> lk(send_ready_mutex_);
      send_ready_cv_.wait_for(lk, std::chrono::microseconds(10000),
                        [&] { return SBCSendReady(round); });

      if (SBCSendReady(round)) {
        break;
      }
    }
    //LOG(ERROR)<<" send ready round ready:"<<round;

    for(int i = 0; i < 10; ++i) {
      //int current_round = proposal_manager_->CurrentRound();
      std::unique_lock<std::mutex> lk(recv_ready_mutex_);
      recv_ready_cv_.wait_for(lk, std::chrono::microseconds(100000),
                        [&] { return SBCRecvReady(round); });
      //if(SBCRecvReady(round)){
      if(proposal_manager_->MayReady(round)){
        break;
      }
    }
    //LOG(ERROR)<<" cehck recv round ready:"<<round;
    proposal_manager_->CheckVote(round-1);
    //LOG(ERROR)<<" notify";
    Notify(proposal_manager_->CurrentRound());
    round++;
  }
}


bool Cassandra::ReceiveProposal(std::unique_ptr<Proposal> proposal) {
  int round = proposal->header().height();
  int proposer = proposal->header().proposer_id();
  //LOG(ERROR)<<" receive proposal round:"<<round<<" proposer:"<<proposer<<" fail mode:"<<failure_mode_;
  if(failure_mode_){
    if(round > 10 && proposer == 1) {
      //LOG(ERROR)<<" reject";
      return true;
    }
  }

  proposal_manager_->AddProposal(std::move(proposal));
  NotifyRecvReady();
  return true;
}

void Cassandra::CommitProposal(std::unique_ptr<Proposal> proposal) {
  //LOG(ERROR)<<" commit proposal, proposer:"<<proposal->header().proposer_id()<<" round:"<<proposal->header().height();
  commit_q_.Push(std::move(proposal));
}

void Cassandra::AsyncCommit() {
  int seq = 1;
  while (!IsStop()) {
    std::unique_ptr<Proposal> proposal = commit_q_.Pop();
    if (proposal == nullptr) {
      continue;
    }
    //LOG(ERROR)<<" proposer commit:"<<proposal->header().proposer_id()<<" round:"<<proposal->header().height()<<" node_info:"<<proposal->node_info_size();
    if(proposal->header().height()>1){
      assert(proposal->node_info_size() > 0);
    }

    {
      int round = proposal->header().height();
      int proposer = proposal->header().proposer_id();
      assert(committed_.find(std::make_pair(proposer, round)) == committed_.end());
      committed_.insert(std::make_pair(proposer, round));
    }

    std::queue<std::unique_ptr<Proposal>> q;
    q.push(std::move(proposal));
    std::vector<std::unique_ptr<Proposal>> list;
    while(!q.empty()){
      std::unique_ptr<Proposal> p = std::move(q.front());
      q.pop();
      //LOG(ERROR)<<" commit queue proposal, proposer:"<<p->header().proposer_id()<<" round:"<<p->header().height();
      for(auto node_info : p->node_info()) {
        int round = node_info.round();
        int proposer = node_info.proposer();
        if(committed_.find(std::make_pair(proposer, round)) != committed_.end()){
          continue;
        }
        committed_.insert(std::make_pair(proposer, round));
        //LOG(ERROR)<<" commit sub proposal:"<<proposer<<" round:"<<round;
        std::unique_ptr<Proposal> np = proposal_manager_->FetchProposal(node_info);
        assert(np != nullptr);
        q.push(std::move(np));
      }
      list.push_back(std::move(p));
    }

    for(auto& p : list) {
      int round = p->header().height();
      int proposer = p->header().proposer_id();
      for (Transaction& txn : *p->mutable_transactions()) {
        txn.set_id(seq++);
        commit_(txn);
      }
    }
  }
}


}  // namespace cassandra_cft
}  // namespace resdb
