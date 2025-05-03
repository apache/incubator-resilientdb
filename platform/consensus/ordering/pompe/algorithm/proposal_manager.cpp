#include "platform/consensus/ordering/pompe/algorithm/proposal_manager.h"

#include <glog/logging.h>
#include "common/utils/utils.h"
#include "common/crypto/signature_verifier.h"

namespace resdb {
namespace pompe {

ProposalManager::ProposalManager(int32_t id, int  f, int limit_count, int n)
    : id_(id), f_(f), limit_count_(limit_count), n_(n) {
    seq_ = 1;
  highTS_.resize(n+1);
  highTSMsgs_.resize(n+1);
  local_ts_ = 0;
  LOG(ERROR)<<" ts size:"<<highTSMsgs_.size();
}

int ProposalManager::GetTimeStamp(){
  std::unique_lock<std::mutex> lk(local_ts_mutex_);
  return local_ts_;
}

void ProposalManager::UpateTimeStamp() {
  std::unique_lock<std::mutex> lk(local_ts_mutex_);
  local_ts_++;
}

int ProposalManager::IncTimeStamp() {
  std::unique_lock<std::mutex> lk(local_ts_mutex_);
  return local_ts_++;
}

std::unique_ptr<Proposal> ProposalManager::GenerateProposal(
    std::vector<std::unique_ptr<Transaction>>& txns) {
  std::unique_ptr<Proposal> proposal = std::make_unique<Proposal>();
  std::string data;
  {
    for(auto& txn: txns){
      *proposal->add_transactions() = *txn;
      std::string tmp;
      txn->SerializeToString(&tmp);
      data += tmp;
    }

    proposal->mutable_header()->set_seq(seq_++);
    proposal->mutable_header()->set_proposer(id_);
    proposal->set_sender(id_);
  }

  std::string header_data;
  proposal->header().SerializeToString(&header_data);
  data +=  header_data;

  std::string hash = SignatureVerifier::CalculateHash(data);
  proposal->set_hash(hash);
  return proposal;
}

void ProposalManager::AddProposal(std::unique_ptr<Proposal> proposal) {
  std::string hash = proposal->hash();
  int seq = proposal->header().seq();
  int proposer = proposal->header().proposer();
  std::unique_lock<std::mutex> lk(local_mutex_);
  data_[std::make_pair(proposer, seq)] = std::move(proposal);
}

std::unique_ptr<Proposal> ProposalManager::FetchProposal(int proposer, int seq) {
  std::unique_lock<std::mutex> lk(local_mutex_);
  auto it = data_.find(std::make_pair(proposer, seq));
  assert(it != data_.end());
  auto ret = std::move(it->second);
  data_.erase(it);
  return ret;
}

void ProposalManager::AddTS(std::unique_ptr<Proposal> proposal) {
  
  int proposer = proposal->header().proposer();
  int sender = proposal->sender();
  int seq = proposal->header().seq();
  int ts = proposal->ts();
  assert(sender == proposer);

  std::unique_lock<std::mutex> lk(ts_mutex_);
  int idx = ts/SlotLen();
  int slot = (idx>0? (idx-1) * n_ + sender : sender);
  //LOG(ERROR)<<" add ts:"<<idx<<" slot:"<<slot;
  if(highTS_[sender] < ts) {
    highTS_[sender] = ts;
    highTSMsgs_[sender] = std::make_unique<Proposal>(*proposal);
  }
  //LOG(ERROR)<<" add ts: seq:"<<seq<< " proposer:"<<proposal->header().proposer()<<" ts:"<<proposal->ts()<<" slot:"<<slot<<" size:"<<localSequencedSet_[slot].size();;
  localSequencedSet_[slot].push_back(std::move(proposal));
}

bool ProposalManager::ContainsHightTS(int64_t global_sync) {
  std::unique_lock<std::mutex> lk(ts_mutex_);
  std::vector<int64_t> ts;
  for(auto & msg : highTSMsgs_) {
    if(msg == nullptr) {
      continue;
    }
    ts.push_back(msg->ts());
  }
  //LOG(ERROR)<<" gloal sync ts:"<<ts.size();
  if(ts.size() < f_+1){
    return false;
  }
  sort(ts.begin(), ts.end(), std::greater<int64_t>());

  //LOG(ERROR)<<" gloal sync ts:"<<ts.size()<<" f:"<<f_<<" ts:"<<ts[f_];
  return ts[f_]>global_sync;
}

std::unique_ptr< TimeStampSync> ProposalManager::GetHightSyncMsg() {
  std::unique_ptr<TimeStampSync> sync = std::make_unique<TimeStampSync>();
  std::unique_lock<std::mutex> lk(ts_mutex_);
  std::vector<int64_t> ts;
  for(auto & msg : highTSMsgs_) {
    if(msg == nullptr) {
      continue;
    }
    *sync->add_ts_msg() = *msg;
    ts.push_back(msg->ts());
  }
  sort(ts.begin(), ts.end(), std::greater<int64_t>());
  sync->set_ts(ts[f_]);
  //LOG(ERROR)<<" gloal set sync ts:"<<ts.size()<<" f:"<<f_<<" ts:"<<ts[f_];
  return sync;
}

int ProposalManager::SlotLen() {
  return 1;
}

std::unique_ptr<Proposal>ProposalManager::GetSlotMsg(int slot) {
  std::unique_ptr<Proposal> p = std::make_unique<Proposal>();

  std::unique_lock<std::mutex> lk(ts_mutex_);
  //LOG(ERROR)<<" get slot msg:"<<slot<<" size:"<<localSequencedSet_[slot].size();
  for(auto &ts : localSequencedSet_[slot]) {
    //LOG(ERROR)<<" get slot msg seq:"<<ts->header().seq()<<" proposer:"<<ts->header().proposer()<<" slot:"<<slot;
    *p->add_ts_msg() = *ts;
  }
  p->set_slot(slot);

  return p;
}

void ProposalManager::RunOne() {
  std::unique_lock<std::mutex> lk(txn_mutex_);
  p_num_++;
}

void ProposalManager::DoneOne() {
  std::unique_lock<std::mutex> lk(txn_mutex_);
  //LOG(ERROR)<<" done one:"<<p_num_;
  p_num_--;
  assert(p_num_>=0);
}

bool ProposalManager::Ready() {
  std::unique_lock<std::mutex> lk(txn_mutex_);
  //LOG(ERROR)<<" check ready:"<<p_num_;
  //return true;
  return p_num_ <100;
}

}  // namespace tusk
}  // namespace resdb
