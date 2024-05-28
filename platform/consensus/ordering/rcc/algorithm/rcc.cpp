#include "platform/consensus/ordering/rcc/algorithm/rcc.h"

#include <glog/logging.h>
#include "common/utils/utils.h"

#define OP0
#define OP2
#define OP4

namespace resdb {
namespace rcc {

RCC::RCC(int id, int f, int total_num, SignatureVerifier* verifier)
    : ProtocolBase(id, f, total_num), verifier_(verifier) {
  proposal_manager_ = std::make_unique<ProposalManager>(id);
  next_seq_ = 1;
  local_txn_id_ = 1;
  execute_id_ = 1;
  totoal_proposer_num_ = total_num_;
  #ifdef OP4
  batch_size_ = 10;
  #else
  batch_size_ = 10;
  #endif
  queue_size_ = 0;
  global_stats_ = Stats::GetGlobalStats();

  send_thread_ = std::thread(&RCC::AsyncSend, this);
  commit_thread_=std::thread(&RCC::AsyncCommit, this);
}

RCC::~RCC() {
  if (send_thread_.joinable()) {
    send_thread_.join();
  }
  if (commit_thread_.joinable()) {
    commit_thread_.join();
  }
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
        //global_stats_->AddCommitWaitingLatency(commit_time - commit_time_[seq]);
    }

    int last = next_seq_;
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
          global_stats_->AddCommitWaitingLatency(commit_time - it.second->queuing_time());
        }
        //LOG(ERROR)<<" commit seq:"<<next_seq_<<" transac num:"<<num<<" block num:"<<pro;
        seq_set_.erase(seq_set_.find(next_seq_));
        global_stats_->AddCommitTxn(num);
        global_stats_->AddCommitBlock(pro);
        #ifdef OP1
        {
          std::unique_lock<std::mutex> lk(seq_mutex_);
          commited_seq_ = std::max(static_cast<int64_t>(next_seq_), commited_seq_);
          //LOG(ERROR)<<"update seq:"<<commited_seq_;
          vote_cv_.notify_all();
        }
        #endif
        next_seq_++;
      } else {
        break;
      }
    }
    int now_seq = next_seq_;
    //LOG(ERROR)<<" execute seq interval:"<<(now_seq-last)<<" now:"<<now_seq<<" last:"<<last<<" next size:"<<seq_set_[next_seq_].size();
  }
}

void RCC::CommitProposal(std::unique_ptr<Proposal> p) {

  {
    p->set_queuing_time(GetCurrentTime());
    if(p->header().proposer_id() == id_) {
    #ifdef OP0
      std::unique_lock<std::mutex> lk(seq_mutex_);
      commited_seq_ = std::max(static_cast<int64_t>(p->header().seq()), commited_seq_);
      //LOG(ERROR)<<"update seq:"<<commited_seq_;
      vote_cv_.notify_all();
    #endif
      global_stats_->AddCommitLatency(GetCurrentTime()- p->header().create_time());
    }
    //std::unique_lock<std::mutex> lk(seq_mutex_);
    //commited_seq_ = std::max(static_cast<int64_t>(p->header().seq()), commited_seq_);
    //vote_cv_.notify_all();
    //global_stats_->AddCommitLatency(GetCurrentTime()- p->header().create_time());
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
    #ifdef OP4
      txn = txns_.Pop(10);
    #else
      txn = txns_.Pop(0);
      #endif
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
    global_stats_->AddBlockSize(txns.size());

    //LOG(ERROR) << "send proposal id:" << id_ << " seq:" << proposal->header().seq();
    broadcast_call_(MessageType::ConsensusMsg, *proposal);
  }
  return;
}

bool RCC::ReceiveProposalList(const Proposal& proposal){
  for(const Proposal& p : proposal.proposals()){
    ReceiveProposal(p);
  }
  return true;
}

bool RCC::ReceiveProposal(const Proposal& proposal) {
  int proposer = proposal.header().proposer_id();
  int64_t seq = proposal.header().seq();
  int sender = proposal.header().sender();
  int status = proposal.header().status();

  std::unique_ptr<Proposal> data;
  std::string hash = proposal.header().hash();

  if (status == ProposalType::NewMsg) {
    data = std::make_unique<Proposal>(proposal);
  }

  bool changed = false;
  {
    //LOG(ERROR) << "received :" << seq
    //            << " from:" << sender
    //            << " proposer:"<< proposer
    //            << " status:" << proposal.header().status()
    //            << " delay:" << GetCurrentTime() - proposal.header().create_time();

    std::unique_lock<std::mutex> lk(mutex_[0]);
    //std::unique_lock<std::mutex> lk(mutex_[proposer]);

    if (status != ProposalType::NewMsg) {
      if (is_commit_[proposer].find(seq) != is_commit_[proposer].end()) {
        return false;
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

  //LOG(ERROR)<<" changed:"<<changed;
  if (changed) {
    std::unique_ptr<Proposal> new_proposal = std::make_unique<Proposal>();
    *new_proposal->mutable_header() = proposal.header();
    new_proposal->mutable_header()->set_sender(id_);
    UpgradeState(new_proposal.get());
    int64_t commit_time = GetCurrentTime();


    

    if (new_proposal->header().status() == ProposalType::Ready_execute) {
      std::unique_lock<std::mutex> lk(mutex_[0]);
      auto it = collector_[proposer].find(hash);
      assert(it != collector_[proposer].end());

      std::unique_ptr<google::protobuf::Message> data = it->second->GetData();

      std::unique_ptr<Proposal> raw_p(dynamic_cast<Proposal*>(data.release()));

      collector_[proposer].erase(it);

      assert(raw_p != nullptr);
      is_commit_[proposer].insert(seq);
      int max_s = 0, min_s = -1;
      for(int i = 1; i <=total_num_;++i){
        int s = 0;
        if(is_commit_[i].empty()){
          //LOG(ERROR)<<" proposer:"<<i<<" comit seq:"<<0;
        }
        else {
          s = *(--is_commit_[i].end());
          //LOG(ERROR)<<" proposer:"<<i<<" comit seq:"<<*(--is_commit_[i].end());
        }
        if(min_s == -1 || min_s > s) min_s = s;
        max_s = max_s > s?max_s:s;
        //LOG(ERROR)<<" proposer:"<<i<<" comit seq:"<<s;
      }
        //LOG(ERROR)<<" proposer gap:"<<max_s- min_s<<" max:"<<max_s<<" min:"<<min_s;
        global_stats_->AddCommitRoundLatency(max_s - min_s);

      // LOG(ERROR)<<"commit type:"<<new_proposal.header().status()<<"
      // transaction size:"<<raw_p->transactions_size();
      CommitProposal(std::move(raw_p));
    } else {
    #ifdef OP2
    std::unique_lock<std::mutex> lk(mutex_[1]);
    //LOG(ERROR)<<" seq:"<<seq<<" last num:"<<(seq==1?0:send_num_[status][seq-1])<<" status:"<<status;
        assert(new_proposal != nullptr);
    if(seq==1 || send_num_[status][seq-1] == total_num_){
      // LOG(ERROR)<<"bc type:"<<new_proposal.header().status();
      //LOG(ERROR)<<" bc seq:"<<seq<<" status:"<<status;
      assert(new_proposal != nullptr);
      Proposal proposal_list;
      
      //Broadcast(MessageType::ConsensusMsg, *new_proposal);
      *proposal_list.add_proposals() = *new_proposal;

      send_num_[status][seq]++;
      if(seq > 2){
        if(send_num_[status].find(seq-2) != send_num_[status].end()){
          send_num_[status].erase(send_num_[status].find(seq-2));
        }
      }
      int64_t next_seq = seq+1;
      while(send_num_[status][next_seq-1] == total_num_){
        if(send_num_[status].find(next_seq-2) != send_num_[status].end()){
          send_num_[status].erase(send_num_[status].find(next_seq-2));
        }
        //LOG(ERROR)<<" find next:"<<next_seq<<" send num:"<<send_num_[status][next_seq];
        auto it = pending_msg_[status].find(next_seq);
        if(it != pending_msg_[status].end()){
          for(auto& p: it->second){
            //LOG(ERROR)<<" bc next:"<<next_seq<<" status:"<<status;
            assert(p != nullptr);
            //Broadcast(MessageType::ConsensusMsg, *p);
            *proposal_list.add_proposals() = *p;
            send_num_[status][next_seq]++;
          }
          pending_msg_[status].erase(it);
          //LOG(ERROR)<<" bc next :"<<next_seq<<" status:"<<status<<" send num:"<<send_num_[status][next_seq];
          next_seq++;
        }
        else {
          break;
        }
      }
          Broadcast(MessageType::ConsensusMsgExt, proposal_list);
      if(status == 0){
      #ifdef OP3
        {
          std::unique_lock<std::mutex> lk(seq_mutex_);
          commited_seq_ = std::max(static_cast<int64_t>(next_seq_), commited_seq_);
          //LOG(ERROR)<<"update seq:"<<commited_seq_;
          vote_cv_.notify_all();
        }
        #endif

      }
    }
      else {
        pending_msg_[status][seq].push_back(std::move(new_proposal));
        //LOG(ERROR)<<" push pending, seq:"<<seq<<" size:"<< pending_msg_[status][seq].size();
      }
      #else
        Broadcast(MessageType::ConsensusMsg, *new_proposal);
      #endif
    }
  }
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
