#include "platform/consensus/ordering/rcc/algorithm/rcc.h"

#include <glog/logging.h>
#include "common/utils/utils.h"

namespace resdb {
namespace rcc {

RCC::RCC(int id, int f, int total_num, SignatureVerifier* verifier)
    : ProtocolBase(id, f, total_num), verifier_(verifier) {
  proposal_manager_ = std::make_unique<ProposalManager>(id);
  next_seq_ = 1;
  local_txn_id_ = 1;
  execute_id_ = 1;
  totoal_proposer_num_ = total_num_;
  batch_size_ = 2;
  queue_size_ = 0;
  global_stats_ = Stats::GetGlobalStats();

  send_thread_ = std::thread(&RCC::AsyncSend, this);
  commit_thread_=std::thread(&RCC::AsyncCommit, this);
}

RCC::~RCC() {
  if (send_thread_.joinable()) {
    send_thread_.join();
  }
  /*
  if (commit_thread_.joinable()) {
    commit_thread_.join();
  }
  */
}

void RCC::AsyncCommit() {
  int64_t last_commit_time  = 0;
  while (!IsStop()) {
    std::unique_ptr<Proposal> msg = execute_queue_.Pop();
    if (msg == nullptr) {
      // LOG(ERROR) << "execu timeout";
      continue;
    }

    int64_t commit_time = GetCurrentTime();
    int64_t waiting_time = commit_time - msg->queuing_time();
    msg->set_queuing_time(commit_time);
    global_stats_->AddCommitQueuingLatency(waiting_time);

    int seq = msg->header().seq();
    int proposer = msg->header().proposer_id();
    if(seq_set_[seq].size()==0){
      commit_time_[seq] = GetCurrentTime();
    }
    //LOG(ERROR)<<" seq:"<<" proposer:"<<proposer<<" wait:"<<waiting_time;
    seq_set_[seq][proposer] = std::move(msg);
    if(seq_set_[seq].size() == totoal_proposer_num_){
        global_stats_->AddCommitWaitingLatency(commit_time - commit_time_[seq]);
    }

    while (!IsStop()) {
      if (seq_set_[next_seq_].size() == totoal_proposer_num_) {
        int64_t commit_time = GetCurrentTime();
        global_stats_->AddCommitInterval(commit_time - last_commit_time);
        last_commit_time = commit_time;
        int num = 0;
        int pro = 0;
        for (auto& it : seq_set_[next_seq_]) {
          pro++;
          //LOG(ERROR)<<"next seq:"<<next_seq_<<" transaction size:"<<it.second->transactions_size()<<" proposer:"<<it.second->header().proposer_id()<<" commit wait:"<<commit_time - it.second->queuing_time();
          for (auto& txn : *it.second->mutable_transactions()) {
            txn.set_id(execute_id_++);
            num++;
            commit_(txn);
          }
          global_stats_->AddCommitRuntime(GetCurrentTime() - commit_time);
          //global_stats_->AddCommitWaitingLatency(commit_time - it.second->queuing_time());
        }
        //LOG(ERROR)<<" commit seq:"<<next_seq_<<" transac num:"<<num<<" block num:"<<pro;
        seq_set_.erase(seq_set_.find(next_seq_));
        global_stats_->AddCommitTxn(num);
        global_stats_->AddCommitBlock(pro);
        {
          std::unique_lock<std::mutex> lk(seq_mutex_);
          commited_seq_ = std::max(static_cast<int64_t>(next_seq_), commited_seq_);
          //LOG(ERROR)<<"update seq:"<<commited_seq_;
          vote_cv_.notify_all();
        }
        next_seq_++;
      } else {
        break;
      }
    }
  }
}

void RCC::CommitProposal(std::unique_ptr<Proposal> p) {

  {
    p->set_queuing_time(GetCurrentTime());
    //std::unique_lock<std::mutex> lk(seq_mutex_);
    //commited_seq_ = std::max(static_cast<int64_t>(p->header().seq()), commited_seq_);
    //vote_cv_.notify_all();
    global_stats_->AddCommitLatency(GetCurrentTime()- p->header().create_time());
    //LOG(ERROR)<<" seq:"<<p->header().seq()<<" proposer:"<<p->header().proposer_id()<<" commit:"<<GetCurrentTime()<<" create time:"<<p->header().create_time()<<" commit time:"<<(GetCurrentTime() - p->header().create_time());
    //global_stats_->AddCommitWaitingLatency(GetCurrentTime() - last_commit_time_);
    //last_commit_time_ = GetCurrentTime();
  }

  execute_queue_.Push(std::move(p));
}

bool RCC::ReceiveTransaction(std::unique_ptr<Transaction> txn) {
  {
    txn->set_create_time(GetCurrentTime());
    std::unique_lock<std::mutex> lk(txn_mutex_);
    txn->set_id(local_txn_id_++);
    txns_.Push(std::move(txn));
    queue_size_++;
  }
  return true;
}

void RCC::AsyncSend() {

  int limit = 2;
  bool start = false;
  int64_t last_time = 0;

  while (!IsStop()) {
    auto txn = txns_.Pop();
    if (txn == nullptr) {
      if(start){
        //LOG(ERROR)<<" not enough txn";
      }
      continue;
    }
    queue_size_--;

    while (!IsStop()) {
      std::unique_lock<std::mutex> lk(seq_mutex_);
      vote_cv_.wait_for(lk, std::chrono::microseconds(1000),
                        [&] { return proposal_manager_->CurrentSeq() - commited_seq_ <= limit; });

      //LOG(ERROR)<<" current seq:"<<proposal_manager_->CurrentSeq()<<" commit seq:"<<commited_seq_;
      if(proposal_manager_->CurrentSeq()  - commited_seq_ <= limit){
        break;
      }
    }
    //LOG(ERROR)<<" committed seq:"<<commited_seq_<<" current:"<<proposal_manager_->CurrentSeq()<<" quueing time:"<<GetCurrentTime() - txn->create_time()<<" queue size:"<<queue_size_;


    txn->set_queuing_time(GetCurrentTime() - txn->create_time());
    global_stats_->AddQueuingLatency(GetCurrentTime() - txn->create_time());

    std::vector<std::unique_ptr<Transaction> > txns;
    txns.push_back(std::move(txn));
    start = true;

    for (int i = 1; i < batch_size_; i++) {
      txn = txns_.Pop(0);
      if (txn == nullptr) {
        break;
      }
      queue_size_--;
      txn->set_queuing_time(GetCurrentTime() - txn->create_time());
      txns.push_back(std::move(txn));
    }

    //int64_t current_time = GetCurrentTime();
    //global_stats_->AddRoundLatency(current_time - last_time);
    //last_time = current_time;
    //LOG(ERROR)<<" propose txn size:"<<txns.size()<<" batch_size_:"<<batch_size_<<" queue size:"<<queue_size_;

    std::unique_ptr<Proposal> proposal =
        proposal_manager_->GenerateProposal(txns);

    //LOG(ERROR) << "send proposal id:" << id_ << " seq:" << proposal->header().seq();
    broadcast_call_(MessageType::ConsensusMsg, *proposal);
  }
  return;
}

bool RCC::ReceiveProposal(const Proposal& proposal) {
  int proposer = proposal.header().proposer_id();
  int seq = proposal.header().seq();
  int sender = proposal.header().sender();
  int status = proposal.header().status();

  std::unique_ptr<Proposal> data;
  std::string hash = proposal.header().hash();

  if (status == ProposalType::NewMsg) {
    data = std::make_unique<Proposal>(proposal);
    //global_stats_->AddCommitLatency(GetCurrentTime() - proposal.header().create_time());
  }

  bool changed = false;
  {
   // LOG(ERROR) << "received :" << seq
   //             << " from:" << sender
   //             << " proposer:"<< proposer
   //             << " status:" << proposal.header().status()
   //             << " delay:" << GetCurrentTime() - proposal.header().create_time();

    std::unique_lock<std::mutex> lk(mutex_[0]);
    //std::unique_lock<std::mutex> lk(mutex_[proposer]);

    if (status != ProposalType::NewMsg) {
      if (is_commit_[proposer].find(seq) != is_commit_[proposer].end()) {
        return 0;
      }
    }

    // LOG(ERROR) << "received :" << seq
    //           << " num:" << received_num_[proposer][seq][status].size()
    //           << " from:" << sender
    //           << " proposer:"<< proposer
    //           << " status:" << proposal.header().status()
    //           << " 2f+1:"<< 2*f_+1;

    auto it = collector_[proposer].find(hash);
    if (it == collector_[proposer].end()) {
      collector_[proposer].insert(
          std::make_pair(hash, std::make_unique<TransactionCollector>(seq)));
      it = collector_[proposer].find(hash);
    } else {
      assert(it->second->GetSeq() == seq);
    }

    it->second->AddRequest(
        status == ProposalType::NewMsg ? std::move(data) : nullptr, sender,
        status,
        [&](const google::protobuf::Message&, int received_count,
            std::atomic<TransactionStatue>* s) {
          if (status == ProposalType::NewMsg || received_count >= 2 * f_ + 1) {
            switch (status) {
              case ProposalType::NewMsg:
                if (*s == TransactionStatue::None) {
                  *s = TransactionStatue::READY_PREPARE;
                  changed = true;
                }
                break;
              case ProposalType::Prepared:
                if (*s == TransactionStatue::READY_PREPARE) {
                  *s = TransactionStatue::READY_COMMIT;
                  changed = true;
                }
                break;
              case ProposalType::Commit:
                if (*s == TransactionStatue::READY_COMMIT) {
                  *s = TransactionStatue::READY_EXECUTE;
                  changed = true;
                }
                break;
            }
          }
          // LOG(ERROR)<<"status:"<<status<<" count:"<<received_count<<"
          // changed:"<<changed;
        });
  }

  if (changed) {
    Proposal new_proposal;
    *new_proposal.mutable_header() = proposal.header();
    new_proposal.mutable_header()->set_sender(id_);
    UpgradeState(&new_proposal);
    int64_t commit_time = GetCurrentTime();
    if (new_proposal.header().status() == ProposalType::Ready_execute) {
      //global_stats_->AddCommitLatency(commit_time - proposal.header().create_time());
      std::unique_lock<std::mutex> lk(mutex_[0]);
      //std::unique_lock<std::mutex> lk(mutex_[proposer]);
      // LOG(ERROR)<<"obtaind ata proposer:"<<proposer<<" seq:"<<seq;
      auto it = collector_[proposer].find(hash);
      assert(it != collector_[proposer].end());

      std::unique_ptr<google::protobuf::Message> data = it->second->GetData();

      std::unique_ptr<Proposal> raw_p(dynamic_cast<Proposal*>(data.release()));

      collector_[proposer].erase(it);

      assert(raw_p != nullptr);
      is_commit_[proposer].insert(seq);

      // LOG(ERROR)<<"commit type:"<<new_proposal.header().status()<<"
      // transaction size:"<<raw_p->transactions_size();
      CommitProposal(std::move(raw_p));
    } else {
      // LOG(ERROR)<<"bc type:"<<new_proposal.header().status();
      Broadcast(MessageType::ConsensusMsg, new_proposal);
    }
  }
  // LOG(ERROR)<<"receive proposal done";
  return true;
}

void RCC::UpgradeState(Proposal* proposal) {
  switch (proposal->header().status()) {
    case ProposalType::NewMsg:
      proposal->mutable_header()->set_status(ProposalType::Prepared);
      return;
    case ProposalType::Prepared:
      proposal->mutable_header()->set_status(ProposalType::Commit);
      return;
    case ProposalType::Commit:
      proposal->mutable_header()->set_status(ProposalType::Ready_execute);
      return;
  }
}

}  // namespace rcc
}  // namespace resdb
