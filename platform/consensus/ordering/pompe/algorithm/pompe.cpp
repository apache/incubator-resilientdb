#include "platform/consensus/ordering/pompe/algorithm/pompe.h"

#include <glog/logging.h>
#include "common/utils/utils.h"


namespace resdb {
namespace pompe {

Pompe::Pompe(
      int id, int f, int total_num, SignatureVerifier* verifier)
      : ProtocolBase(id, f, total_num), replica_num_(total_num){
  
   LOG(ERROR)<<"id:"<<id<<" f:"<<f<<" total:"<<replica_num_;
  last_slot_ = 0;
  next_slot_ = 0;
  primary_ = 1;
  batch_size_ = 1;
  globalSyncTS_ = 0;
  next_commit_slot_=0;
  n_ = replica_num_;

  proposal_manager_ = std::make_unique<ProposalManager>(id, f, 2*f_+1, total_num);
  global_stats_ = Stats::GetGlobalStats();
  send_thread_ = std::thread(&Pompe::AsyncSend, this);
  consensu_thread_ = std::thread(&Pompe::AsyncCommit, this);
  sync_thread_ = std::thread(&Pompe::SyncTs, this);
  //LOG(ERROR)<<" init done";
}

Pompe::~Pompe() {
}

bool Pompe::ReceiveTransaction(std::unique_ptr<Transaction> txn) {
  //LOG(ERROR)<<" get transaction";
  txn->set_proposer(id_);
  txns_.Push(std::move(txn));
  return true;
}

void Pompe::AsyncSend() {
  while (!IsStop()) {

    {
      std::unique_lock<std::mutex> lk(cv_mutex_);
      vote_cv_.wait_for(lk, std::chrono::microseconds(1000),
          [&] { return proposal_manager_->Ready(); });

      if(!proposal_manager_->Ready()){
        continue;
      }
    }

    SendProposal();
  }
}

void Pompe::SendProposal() {
  auto  txn = txns_.Pop();
  if(txn == nullptr) {
    return;
  }

  std::vector<std::unique_ptr<Transaction> > txns;
  txns.push_back(std::move(txn));
  while(txns.size() <batch_size_){
    auto txn = txns_.Pop();
    if(txn == nullptr){
      continue;
      //break;
    }
    txns.push_back(std::move(txn));
  }

  std::unique_ptr<Proposal>  proposal = proposal_manager_->GenerateProposal(txns);
  //LOG(ERROR)<<" bc proposal seq:"<<proposal->header().seq();
  Broadcast(MessageType::NewProposal, *proposal);
  proposal_manager_->RunOne();
}

void Pompe::ReceiveProposal(std::unique_ptr<Proposal> proposal) {
  int sender = proposal->sender();
  int seq = proposal->header().seq();
  Proposal proposal_ack;
  *proposal_ack.mutable_header() = proposal->header();
  proposal_ack.set_ts(proposal_manager_->IncTimeStamp());
  proposal_ack.set_sender(id_);

  //LOG(ERROR)<<" receive proposal from:"<<sender<<" seq:"<<seq;

  proposal_manager_->AddProposal(std::move(proposal));

  SendMessage(MessageType::ProposalACK, proposal_ack, sender);
}

void Pompe::ReceiveProposalACK(std::unique_ptr<Proposal> proposal) {
  int sender = proposal->sender();
  int seq = proposal->header().seq();
  int proposer = proposal->header().proposer();
  //proposal->set_ts(GetCurrentTime());
  std::unique_lock<std::mutex> lk(mutex_);
  int64_t start_time = proposal->header().start_time();
  //LOG(ERROR)<<" receive proposal ack from:"<<sender<<" seq:"<<seq<< " proposer:"<<proposal->header().proposer()<<" ts:"<<proposal->ts()<<" proposer:"<<proposer;
  received_[seq][sender] = std::move(proposal);
  if(static_cast<int>(received_[seq].size()) == 2*f_+1){
    Proposal proposal_ts;
    *proposal_ts.mutable_header() = received_[seq][sender]->header();
    proposal_ts.set_sender(id_);
    AssignTs(&proposal_ts);
    Broadcast(MessageType::ProposalTS, proposal_ts);
    LOG(ERROR)<<" proposal ts run time:"<<(GetCurrentTime() - start_time);
  }
}

void Pompe::AssignTs(Proposal * proposal) {
  int seq = proposal->header().seq();
  std::vector<int64_t> ts;
  for(auto & it : received_[seq]){
    auto& p = it.second;
    ts.push_back(p->ts());
    *proposal->add_cert() = *p;
  }

  sort(ts.begin(), ts.end());
  int64_t assigned_ts = ts[f_+1];
  proposal->set_ts(assigned_ts);
  //LOG(ERROR)<<" assign ts: seq:"<<seq<< "proposer:"<<proposal->header().proposer()<<" ts:"<<proposal->ts();
}

void Pompe::ReceiveProposalTS(std::unique_ptr<Proposal> proposal) {
 // Verify
  proposal->clear_cert();
  int64_t start_time = proposal->header().start_time();
  proposal_manager_->AddTS(std::move(proposal));
  LOG(ERROR)<<" proposal recv ts run time:"<<(GetCurrentTime() - start_time);
}

void Pompe::SyncTs(){
  int last_ts = -1;
  while(!IsStop()){
    {
      std::unique_lock<std::mutex> lk(sync_mutex_);
      if(last_ts != globalSyncTS_ && proposal_manager_->ContainsHightTS(globalSyncTS_)){
        std::unique_ptr<TimeStampSync> sync = proposal_manager_->GetHightSyncMsg();
        sync->set_sender(id_);
        //LOG(ERROR)<<" sync ts:"<<sync->ts()<<" global ts:"<<globalSyncTS_;
        Broadcast(MessageType::SyncTS, *sync);
        last_ts = globalSyncTS_;
        continue;
      }
    }
    usleep(10000);
  }
}

void Pompe::ReceiveSyncTS(std::unique_ptr<TimeStampSync> sync) {
  std::unique_lock<std::mutex> lk(sync_mutex_);
  //LOG(ERROR)<<" receive ts:"<<sync->ts()<<" from:"<<sync->sender()<<" global:"<<globalSyncTS_;
  if(globalSyncTS_ < sync->ts()){
    globalSyncTS_ = sync->ts();
    localSyncTS_ = GetCurrentTime();
    //LOG(ERROR)<<" global sync ts:"<<sync->ts()<<" update global ts:"<<globalSyncTS_;
    TryCollect();
  }
}

void Pompe::TryCollect() {
//LOG(ERROR)<<__func__;
  int idx = globalSyncTS_/proposal_manager_->SlotLen();
  if(idx == 0) return;
  int current_slot = (idx-1)*n_;
  //int current_slot = (globalSyncTS_/proposal_manager_->SlotLen()-1)*n_;
  //LOG(ERROR)<<" try collect global ts:"<<globalSyncTS_<<" current slot:"<<current_slot<<" idx:"<<idx;
  if(current_slot <= last_slot_) {
    return;
  }

  //current_slot = std::min(current_slot, last_slot_+1);
  for(int i = last_slot_; i < current_slot; ++i){
    auto p = proposal_manager_->GetSlotMsg(i);
    p->set_sender(id_);
    assert(p != nullptr);
    SendMessage(MessageType::Collect, *p, primary_);
  }
  //LOG(ERROR)<<" get slot done"<<" last:"<<last_slot_<<" current slot:"<<current_slot;
  last_slot_ = current_slot;
  //LOG(ERROR)<<__func__<<" done";
}

void Pompe::ReceiveCollect(std::unique_ptr<Proposal> proposal) {
//LOG(ERROR)<<__func__;
  int sender = proposal->sender();
  int slot = proposal->slot();
  std::unique_lock<std::mutex> lk(slot_mutex_);
  //LOG(ERROR)<<" receive collect <sender:"<<sender<<" slot:"<<slot<<" next_slot:"<<next_slot_;
  pending_proposal_[slot][sender]= std::move(proposal);
  while(static_cast<int>(pending_proposal_[next_slot_].size()) >= 2*f_+1) {
    DoConsensus();
  }
//LOG(ERROR)<<__func__<<" done";
}

void Pompe::DoConsensus() {
//LOG(ERROR)<<__func__;
  std::unique_ptr<Proposal> proposal = std::make_unique<Proposal>();
  for(auto &p : pending_proposal_[next_slot_]) {
    //LOG(ERROR)<<" collect proposer:"<<p.first<<" slot:"<<next_slot_;
    *proposal->add_cert() = *p.second;
  }
  //LOG(ERROR)<<" propose:"<<next_slot_;
  global_stats_->IncPropose();
  proposal->set_slot(next_slot_);
  Broadcast(MessageType::Propose, *proposal);
  next_slot_++;
//LOG(ERROR)<<__func__<<" done";
}

void Pompe::ReceivePropose(std::unique_ptr<Proposal> proposal) {
//LOG(ERROR)<<__func__;
  int slot = proposal->slot();
  std::unique_lock<std::mutex> lk(propose_mutex_);
  //LOG(ERROR)<<" receive propose from:"<<slot;
  propose_data_[slot] = std::move(proposal);

  Proposal new_proposal;
  new_proposal.set_slot(slot);
  new_proposal.set_sender(id_);
  Broadcast(MessageType::Prepare, new_proposal);
//LOG(ERROR)<<__func__<<" done";
}

void Pompe::ReceivePrepare(std::unique_ptr<Proposal> proposal) {
//LOG(ERROR)<<__func__;
  int slot = proposal->slot();
  int sender = proposal->sender();
  //LOG(ERROR)<<" receive prepare from:"<<slot<<" sender:"<<sender;
  std::unique_lock<std::mutex> lk(prepare_mutex_);
  prepare_received_[slot].insert(sender);
  if(static_cast<int>(prepare_received_[slot].size()) == 2*f_+1){
    proposal->set_sender(id_);
    Broadcast(MessageType::Commit, *proposal);
  }
//LOG(ERROR)<<__func__<<" done";
}

void Pompe::ReceiveCommit(std::unique_ptr<Proposal> proposal) {
//LOG(ERROR)<<__func__;
  int slot = proposal->slot();
  int sender = proposal->sender();
  int64_t start_time = proposal->header().start_time();
  std::unique_lock<std::mutex> lk(commit_mutex_);
  //LOG(ERROR)<<" receive commit from:"<<slot<<" sender:"<<sender;
  commit_received_[slot].insert(sender);
  if(static_cast<int>(commit_received_[slot].size()) == 2*f_+1){
    LOG(ERROR)<<" proposal recv ts run time:"<<(GetCurrentTime() - start_time)<<" start time:"<<start_time;
    CommitSlot(slot);
  }
//LOG(ERROR)<<__func__<<" done";
}

void Pompe::CommitSlot(int slot) {
//LOG(ERROR)<<" commit slot:"<<slot;
  commit_slot_.insert(slot);
  while(!commit_slot_.empty() && *commit_slot_.begin() == next_commit_slot_){
    commit_queue_.Push(std::make_unique<int>(next_commit_slot_));
    commit_slot_.erase(commit_slot_.begin());
    next_commit_slot_++;
  }
}

void Pompe::AsyncCommit() {
  int exe_seq = 1;
  while(!IsStop()){
    auto p_slot = commit_queue_.Pop();
    if(p_slot == nullptr ) {
      continue;
    }
    int slot = *p_slot;

    //LOG(ERROR)<<" commit slot:"<<slot;
    std::unique_ptr<Proposal> p_list = nullptr;
    while(p_list == nullptr) {
      std::unique_lock<std::mutex> lk(propose_mutex_);
      p_list = std::move(propose_data_[slot]);
    }
    assert(p_list != nullptr);

    // <ts, proposer>
    std::map<std::pair<int,int>, std::set<int> >list;
    bool need_notify = false;
    for(auto& p : p_list->cert()) {
      for(auto& msg : p.ts_msg()){
        int ts = msg.ts();
        int proposer = msg.header().proposer();
        //LOG(ERROR)<<" commit collect proposer:"<<msg.header().proposer()<<" seq:"<< msg.header().seq()<<" slot:"<<slot;
        list[std::make_pair(ts, proposer)].insert(msg.header().seq());
      }
    }

    int num = 0;
    for(auto it : list) {
      int proposer =it.first.second;
      for(auto seq: it.second){
        auto p = proposal_manager_->FetchProposal(proposer, seq);
        if(proposer == id_) {
          //LOG(ERROR)<<" commit seq:"<<seq<<" proposer:"<<proposer<<" slot:"<<slot;
          need_notify = true;
          proposal_manager_->DoneOne();
        }
        for(auto & txn : *p->mutable_transactions()){
          txn.set_id(exe_seq++);
          num++;
          Commit(txn);
        }
      }
    }
    if(need_notify) {
      std::unique_lock<std::mutex> lk(cv_mutex_);
      vote_cv_.notify_all();
    }
    //LOG(ERROR)<<" commit slot:"<<slot<<" done"<<" size:"<<num;
  }
}

}  // namespace tusk
}  // namespace resdb
