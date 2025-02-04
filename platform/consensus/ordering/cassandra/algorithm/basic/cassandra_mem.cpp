#include "platform/consensus/ordering/cassandra/algorithm/cassandra_mem.h"

#include <glog/logging.h>

#include "common/crypto/signature_verifier.h"
#include "common/utils/utils.h"

namespace resdb {
namespace cassandra {

CassandraMem::CassandraMem(
    int id, int batch_size, int total_num, int f,
    std::function<int(int, google::protobuf::Message& msg, int)> single_call,
    std::function<int(int, google::protobuf::Message& msg)> broadcast_call,
    std::function<int(const Transaction& txn)> commit) {
  LOG(ERROR) << "get proposal graph";
  id_ = id;
  total_num_ = total_num;
  f_ = f;
  is_stop_ = false;
  timeout_ms_ = 60000;
  local_txn_id_ = 1;
  local_proposal_id_ = 1;
  batch_size_ = batch_size;
  single_call_ = single_call;
  broadcast_call_ = broadcast_call;
  commit_ = commit;

  recv_num_ = 0;
  execute_num_ = 0;
  executed_ = 0;
  committed_num_ = 0;
  precommitted_num_ = 0;

  graph_ = std::make_unique<ProposalGraph>(f_);
  proposal_manager_ = std::make_unique<ProposalManager>(id, graph_.get());

  graph_->SetCommitCallBack(
      [&](const Proposal& proposal) { CommitProposal(proposal); });

  Reset();

  consensus_thread_ = std::thread(&CassandraMem::AsyncConsensus, this);

  commit_thread_ = std::thread(&CassandraMem::AsyncCommit, this);
}

CassandraMem::~CassandraMem() {
  is_stop_ = true;
  if (consensus_thread_.joinable()) {
    consensus_thread_.join();
  }
  if (commit_thread_.joinable()) {
    commit_thread_.join();
  }
}

void CassandraMem::Reset() {
  // received_num_.clear();
  // state_ = State::NewProposal;
}

void CassandraMem::AsyncConsensus() {
  int height = 0;
  while (!is_stop_) {
    height = SendTxn();
    if (height == -1) {
      usleep(1000000);
      continue;
    }
    WaitVote(height);
  }
}

bool CassandraMem::WaitVote(int height) {
  std::unique_lock<std::mutex> lk(mutex_);
  /*
  if (height <= 1)
    timeout_ms_ = 20000;
  else
    timeout_ms_ = 10000;
    */
  vote_cv_.wait_for(lk, std::chrono::microseconds(timeout_ms_ * 1000),
                    [&] { return can_vote_[height]; });
  if (!can_vote_[height]) {
    LOG(ERROR) << "wait vote time out"
               << " can vote:" << can_vote_[height] << " height:" << height;
  }
  return true;
}

void CassandraMem::AsyncCommit() {
  while (!is_stop_) {
    std::unique_ptr<Proposal> msg = execute_queue_.Pop(timeout_ms_ * 1000);
    if (msg == nullptr) {
      LOG(ERROR) << "execu timeout";
      continue;
    }
    {
      // std::unique_lock<std::mutex> lk(mutex_);
      executed_++;
      if (msg->header().proposer_id() == id_) {
        execute_num_ += msg->transactions_size();
      }

      LOG(ERROR) << "!!! execute:" << msg->transactions_size()
                 << " execute:" << execute_num_
                 << " committed:" << committed_num_ << " executed:" << executed_
                 << " commit delay:" << GetCurrentTime() - msg->create_time();

      /*
      int running_size = 0;
      for(auto& it : uncommitted_txn_){
        running_size+=it.second.size();
      }

      LOG(ERROR)<<"!!! execute:"<<
        msg->transactions_size()
        <<" recv:"<<recv_num_
        <<" execute:"<<execute_num_
        <<" committed:"<<committed_num_
        <<" executed:"<<executed_
        <<" pool size:"<<txns_.size()
        <<" running:"<<running_size
        <<" total in men:"<<running_size+txns_.size()
        <<" total:"<<running_size+txns_.size()+execute_num_+pending_num_
        <<" out of uncommit:"<<precommitted_num_
        <<" mining num:"<<pending_num_
        <<" total txn:"<<precommitted_num_+running_size+txns_.size() +
      pending_num_;
        */
    }
    for (const Transaction& txn : msg->transactions()) {
      commit_(txn);
    }
  }
}

void CassandraMem::CommitProposal(const Proposal& p) {
  LOG(ERROR) << "commit proposal from proposer:" << p.header().proposer_id()
             << " id:" << p.header().proposal_id()
             << " height:" << p.header().height()
             << " transaction size:" << p.transactions_size();

  {
    // std::unique_lock<std::mutex> lk(mutex_);
    auto txn_it = uncommitted_txn_.find(p.header().height());
    // LOG(ERROR)<<"right now size:"<<txns_.size();
    if (txn_it != uncommitted_txn_.end()) {
      // LOG(ERROR)<<"un commit txn size:"<<txn_it->second.size()<<"
      // height:"<<p.header().height();
      for (auto& txn : txn_it->second) {
        if (p.header().proposer_id() != id_) {
          // LOG(ERROR)<<"put back txn id:"<<txn->id();
          txns_.push_back(std::move(txn));
        } else {
          precommitted_num_++;
        }
      }
      uncommitted_txn_.erase(txn_it);
    }
    // LOG(ERROR)<<"commit done txn size:"<<txns_.size();
    int sum = 0;
    for (auto& it : uncommitted_txn_) {
      // LOG(ERROR)<<"height:"<<it.first<<" contain:"<<it.second.size();
      sum += it.second.size();
    }
    assert(uncommitted_txn_.empty() ||
           uncommitted_txn_.begin()->first > p.header().height());
    LOG(ERROR) << " in pending size:" << sum << " pool size:" << txns_.size();
  }
  committed_num_++;
  LOG(ERROR) << "commit num:" << committed_num_
             << " commit delay:" << GetCurrentTime() - p.create_time();
  execute_queue_.Push(std::make_unique<Proposal>(p));
}

bool CassandraMem::ReceiveTransaction(std::unique_ptr<Transaction> txn) {
  {
    std::unique_lock<std::mutex> lk(mutex_);
    txn->set_id(local_txn_id_++);
    txns_.Push(std::move(txn));
    recv_num_++;
  }
  return true;
}

void CassandraMem::BroadcastTxn() {
  std::vector<Transaction*> txns;
  while (!IsStop()) {
    std::unique_ptr<Transaction> txn = txns_.Pop();
    if (txn == nullptr) {
      continue;
    }

    txns.push_back(std::move(txn));
    if (txns.size() < batch_size_) {
      continue;
    }
    auto block = proposal_manager_->MakeBlock(txns);
    broadcast_call_(MessageType::NewBlocks, *block);
    txns.clear();
  }
}

void CassandraMem::ReceiveBlocks(const Proposal& proposal) {
  proposal_manager_->AddBlocks(proposal);
}

int CassandraMem::SendTxn() {
  std::vector<Transaction*> txns;
  std::vector<std::unique_ptr<Transaction>> pending;
  {
    std::unique_lock<std::mutex> lk(mutex_);
    if (txns_.empty()) {
      LOG(ERROR) << "no transactions";
      if (start_ == false) {
        return -1;
        return graph_->GetCurrentHeight() + 1;
      }
      // return -1;
    }
    start_ = true;
    auto it = txns_.begin();
    for (int i = 0; i < batch_size_ && it != txns_.end(); ++i) {
      txns.push_back(it->get());
      pending.push_back(std::move(*it));
      it++;
      txns_.pop_front();
    }
    pending_num_ = pending.size();
  }
  std::unique_ptr<Proposal> proposal =
      proposal_manager_->GenerateProposal(txns);
  {
    std::unique_lock<std::mutex> lk(mutex_);
    // LOG(ERROR)<<"generate proposal size:"<<txns.size()<<"
    // left:"<<txns_.size()<<" height:"<<proposal->header().height();
    for (auto& it : pending) {
      uncommitted_txn_[proposal->header().height()].push_back(std::move(it));
    }
    pending_num_ = 0;
  }

  LOG(ERROR) << "send proposal id:" << id_
             << " proposal id:" << proposal->header().proposal_id();
  broadcast_call_(MessageType::NewProposal, *proposal);
  return proposal->header().height();
}

bool CassandraMem::ReceiveProposal(const Proposal& proposal) {
  {
    std::unique_lock<std::mutex> lk(mutex_);
    const Proposal* pre_p =
        graph_->GetProposalInfo(proposal.header().prehash());
    if (pre_p == nullptr) {
      LOG(ERROR) << "receive proposal from :" << proposal.header().proposer_id()
                 << " id:" << proposal.header().proposal_id() << "no pre:";
    } else {
      LOG(ERROR) << "receive proposal from :" << proposal.header().proposer_id()
                 << " id:" << proposal.header().proposal_id()
                 << "pre:" << pre_p->header().proposer_id()
                 << " pre id:" << pre_p->header().proposal_id();
    }
    if (!proposal_manager_->VerifyProposal(proposal)) {
      LOG(ERROR) << "proposal is invalid receive proposal from :"
                 << proposal.header().proposer_id()
                 << " id:" << proposal.header().proposal_id();
      return false;
    }
    LOG(ERROR) << "add proposal";
    if (!graph_->AddProposal(proposal)) {
      LOG(ERROR) << "add proposal fail";
      // TrySendRecoveery(proposal);
      return false;
    }

    received_num_[proposal.header().height()].insert(
        proposal.header().proposer_id());
    LOG(ERROR) << "received current height:" << graph_->GetCurrentHeight()
               << " num:" << received_num_[graph_->GetCurrentHeight()].size()
               << " from:" << proposal.header().proposer_id()
               << " last vote:" << last_vote_;
    if (received_num_[graph_->GetCurrentHeight()].size() == total_num_) {
      if (last_vote_ < graph_->GetCurrentHeight()) {
        last_vote_ = graph_->GetCurrentHeight();
        can_vote_[graph_->GetCurrentHeight()] = true;
        vote_cv_.notify_all();
      }
    }

    std::vector<std::unique_ptr<Proposal>> future_g = graph_->GetNotFound(
        proposal.header().height() + 1, proposal.header().hash());
    if (future_g.size() > 0) {
      LOG(ERROR) << "get future size:" << future_g.size();
      for (auto& it : future_g) {
        if (!graph_->AddProposal(*it)) {
          LOG(ERROR) << "add future proposal fail";
          // TrySendRecoveery(proposal);
          continue;
        }

        received_num_[it->header().height()].insert(it->header().proposer_id());
        LOG(ERROR) << "received current height:" << graph_->GetCurrentHeight()
                   << " num:"
                   << received_num_[graph_->GetCurrentHeight()].size()
                   << " from:" << it->header().proposer_id()
                   << " last vote:" << last_vote_;
      }
    }
  }
  // LOG(ERROR)<<"receive proposal done";
  return true;
}

}  // namespace cassandra
}  // namespace resdb
