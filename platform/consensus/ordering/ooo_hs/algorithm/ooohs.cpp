#include "platform/consensus/ordering/ooo_hs/algorithm/ooohs.h"

#include <queue>
#include <glog/logging.h>
#include "common/utils/utils.h"


namespace resdb {
namespace ooohs {

OOOHotStuff::OOOHotStuff(int id, int f, int total_num, SignatureVerifier * verifier)
  : ProtocolBase(id, f, total_num), verifier_(verifier){

    slot_num_ = 5;
    LOG(ERROR)<<"id:"<<id<<" f:"<<f<<" total:"<<total_num_<<" slot num:"<<slot_num_;

  proposal_manager_ = std::make_unique<ProposalManager>(id, 2*f_+1, slot_num_, verifier);
  has_sent_.resize(slot_num_);
  for(int i = 0; i < slot_num_; ++i){
    has_sent_[i] = 0;
  }

  for(int i = 0; i < slot_num_; ++i){
    send_thread_.push_back(std::thread(&OOOHotStuff::AsyncSend, this,i));
  }

    commit_thread_ = std::thread(&OOOHotStuff::AsyncCommit, this);
    batch_size_ = 1;
}

OOOHotStuff::~OOOHotStuff() {
}

int OOOHotStuff::NextLeader(int view){
  //LOG(ERROR)<<" view:"<<view<<" next leader:"<<(view+1)%total_num_ + 1;
  return (view+1)%total_num_ + 1;
}

bool OOOHotStuff::IsLeader(int view){
  return (view % total_num_)+1 == id_;
}

bool OOOHotStuff::Ready(int k) {
  int view = proposal_manager_->CurrentView(k);
  //LOG(ERROR)<<"get view:"<<view<<" from slot:"<<k<<" has sent:"<<has_sent_[k];
  return IsLeader(view) && has_sent_[k]!=view;
}

void OOOHotStuff::StartNewRound() {
  std::unique_lock<std::mutex> lk(n_mutex_);
  vote_cv_.notify_all();
  //LOG(ERROR)<<" start new round";
}

void OOOHotStuff::AsyncSend(int slot) {
  //LOG(ERROR)<<"start slot:"<<slot;
  bool started = false;
  while (!IsStop()) {
    auto txn = txns_.Pop();
    if(txn == nullptr){
      if(started){
        LOG(ERROR)<<" wait txn:";
      }
      continue;
    }

    while(!IsStop()){
      std::unique_lock<std::mutex> lk(n_mutex_);
      vote_cv_.wait_for(lk, std::chrono::microseconds(1000),
          [&] { return Ready(slot); });

      //LOG(ERROR)<<"check slot:"<<slot;
      if(Ready(slot)){
        break;
      }
    }
    if(IsStop()){
      return;
    }

    std::vector<std::unique_ptr<Transaction> > txns;
    txns.push_back(std::move(txn));
    for(int i = 1; i < batch_size_; ++i){
      auto txn = txns_.Pop(0);
      if(txn == nullptr){
        break;
      }
      txns.push_back(std::move(txn));
    }

    std::unique_ptr<Proposal> proposal =  nullptr;
    {
      std::unique_lock<std::mutex> lk(mutex_);
      proposal = proposal_manager_ -> GenerateProposal(txns, slot);
      //LOG(ERROR)<<"propose view:"<<proposal->header().view();
    }
    has_sent_[slot] = proposal->header().view();
    broadcast_call_(MessageType::NewProposal, *proposal);
  }
}

void OOOHotStuff::AsyncCommit() {
  std::pair<int,int> next_execute_key = std::make_pair(1,0);
  int64_t seq = 1;
  while (!IsStop()) {
    auto p = commit_q_.Pop();
    if(p == nullptr){
      continue;
    }

    int view = p->header().view();
    int slot = p->header().slot();
    //LOG(ERROR)<<"receive commit proposal view:"<<view<<" slot:"<<slot;
    pending_commit_[std::make_pair(view, slot)] = std::move(p);

    while(true){
      if(pending_commit_.find(next_execute_key) != pending_commit_.end()){
        auto exe_p = pending_commit_.find(next_execute_key);
        assert(exe_p != pending_commit_.end());
        //LOG(ERROR)<<"commit proposal view:"<<exe_p->second->header().view()<<" slot:"<<exe_p->second->header().slot();
        for(Transaction& txn : *exe_p->second->mutable_transactions()){
          txn.set_id(seq++);
          Commit(txn);
        }
        pending_commit_.erase(exe_p);
        next_execute_key.second++;
        if(next_execute_key.second == slot_num_){
          next_execute_key.first++;
          next_execute_key.second = 0;
        }
      }
      else {
        break;
      }
    }
  }
}


bool OOOHotStuff::ReceiveTransaction(std::unique_ptr<Transaction> txn) {
  //  std::unique_lock<std::mutex> lk(txn_mutex_);
  txn->set_proposer(id_);
  txns_.Push(std::move(txn));
  return true;
}

bool OOOHotStuff::ReceiveProposal(std::unique_ptr<Proposal> proposal) {
    int view = proposal->header().view();
    int slot = proposal->header().slot();

    //LOG(ERROR)<<"RECEIVE proposer view:"<<view<<" slot:"<<slot;
    std::unique_lock<std::mutex> lk(p_mutex_);
    proposals_[std::make_pair(view,slot)]=std::move(proposal);

    std::queue<std::pair<int, int>> q;
    q.push(std::make_pair(view,slot));

    auto IsReady = [&](int view, int slot){
      if(view>1){
        std::pair<int,int> pre_view_bucket = std::make_pair(view-1, slot);
        if(proposals_.find(pre_view_bucket) == proposals_.end()){
          return false;
        }
        if(proposals_[pre_view_bucket] != nullptr){
          return false;
        }
      }

      if(slot>0){
        std::pair<int,int> pre_slot_bucket = std::make_pair(view, slot-1);
        if(proposals_.find(pre_slot_bucket) == proposals_.end()){
          return false;
        }
        if(proposals_[pre_slot_bucket] != nullptr){
          return false;
        }
      }
      return true;

    };

    while(!q.empty()){
      std::pair<int,int> bucket = q.front();
      q.pop();
      int view = bucket.first;
      int slot = bucket.second;
      //LOG(ERROR)<<"check proposer view:"<<view<<" slot:"<<slot;
      if(!IsReady(view, slot)){
       // LOG(ERROR)<<"proposal view:"<<view<<" slot:"<<slot<<" is not ready";
        continue;
      }

      auto it = proposals_.find(bucket);
      assert(it != proposals_.end());
      if(it->second == nullptr){
        continue;
      }
      ProcessProposal(std::move(it->second));

      {
        //clean up
        /*
           {
           auto cit = proposals_.find(std::make_pair(view-2, slot));
           if(cit != proposals_.end()){
           proposals_.erase(cit);
           }
           }
           {
           auto cit = proposals_.find(std::make_pair(view, slot-2));
           if(cit != proposals_.end()){
           proposals_.erase(cit);
           }
           }
         */
      }

      std::pair<int,int> next_slot_bucket = std::make_pair(view, slot+1);
      {
        auto it = proposals_.find(next_slot_bucket);
        if(it != proposals_.end() && it->second != nullptr){
          q.push(next_slot_bucket);
        }
      }

      std::pair<int,int> next_view_bucket = std::make_pair(view+1, slot);
      {
        auto it = proposals_.find(next_view_bucket);
        if(it != proposals_.end() && it->second != nullptr){
          q.push(next_view_bucket);
        }
      }
    }
    //LOG(ERROR)<<"RECEIVE proposer view:"<<view<<" slot:"<<slot<<" done";

  return true;
}

bool OOOHotStuff::ProcessProposal(std::unique_ptr<Proposal> proposal) {
    int view = proposal->header().view();
    int slot = proposal->header().slot();
    std::unique_ptr<Certificate> cert = nullptr;
  {
    //LOG(ERROR)<<"process proposer view:"<<view<<" slot:"<<slot;
    std::unique_lock<std::mutex> lk(mutex_);
    if(!proposal_manager_->Verify(*proposal)){
      LOG(ERROR)<<" proposal invalid";
      return false;
    }

    cert = GenerateCertificate(*proposal);
    assert(cert != nullptr);

    std::vector<std::unique_ptr<Proposal>> committed_list = proposal_manager_->AddProposal(std::move(proposal));
    for(auto& p : committed_list){
      if(p != nullptr){
        CommitProposal(std::move(p));
      }
    }

    //LOG(ERROR)<<"send cert view:"<<view<<" slot:"<<slot<<" to:"<<NextLeader(view)<<" commit proposal size:"<<committed_list.size();
  }

  SendMessage(MessageType::Vote, *cert, NextLeader(view));

  return true;
}


bool OOOHotStuff::ReceiveCertificate(std::unique_ptr<Certificate> cert) {

  int view = cert->view();
  int slot = cert->slot();

  //LOG(ERROR)<<"RECEIVE proposer cert :"<<cert->view()<<" from:"<<cert->signer();
  std::unique_lock<std::mutex> lk(mutex_);
  bool valid = proposal_manager_->VerifyCert(*cert);
  if(!valid){
    LOG(ERROR) << "Verify message fail";
    assert(1==0);
    return false;
  }

  std::pair<int,int> key = std::make_pair(view, slot);
  std::string hash = cert->hash();
  receive_[key][hash].insert(std::make_pair(cert->signer(), std::move(cert)));

  //LOG(ERROR)<<"RECEIVE proposer cert, view :"<<view<<" slot:"<<slot<<" size:"<<receive_[key][hash].size();
  if(receive_[key][hash].size() == 2*f_+1){
    std::unique_ptr<QC> qc = std::make_unique<QC>();
    qc->set_hash(hash);
    qc->set_view(view);
    qc->set_slot(slot);

    for(auto & it: receive_[key][hash]){
      *qc->add_signatures() = it.second->sign();
    }

    //LOG(ERROR)<<"add qc view:"<<view<<" slot:"<<slot;
    proposal_manager_->AddQC(std::move(qc));

    //LOG(ERROR)<<"start new round";
    StartNewRound();
  }
  return true;
}

void OOOHotStuff::CommitProposal(std::unique_ptr<Proposal> p){
  commit_q_.Push(std::move(p));
}

std::unique_ptr<Certificate> OOOHotStuff::GenerateCertificate(const Proposal& proposal) {
  std::unique_ptr<Certificate> cert = std::make_unique<Certificate>();
  cert->set_hash(proposal.hash());
  cert->set_view(proposal.header().view());
  cert->set_slot(proposal.header().slot());
  cert->set_signer(id_);

  std::string data_str = proposal.hash();
  auto hash_signature_or = verifier_->SignMessage(data_str);
  if (!hash_signature_or.ok()) {
    LOG(ERROR) << "Sign message fail";
    return nullptr;
  }
  *cert->mutable_sign()=*hash_signature_or;
  return cert;
}

}  // namespace tusk
}  // namespace resdb
