#pragma once

#include "platform/consensus/ordering/pompe/proto/proposal.pb.h"

namespace resdb {
namespace pompe {

class ProposalManager {
 public:
  ProposalManager(int32_t id, int f, int limit_count, int n);

  std::unique_ptr<Proposal> GenerateProposal(std::vector<std::unique_ptr<Transaction>>& txns);

  void AddProposal(std::unique_ptr<Proposal> proposal);
  std::unique_ptr<Proposal> FetchProposal(int proposer, int seq);

  void AddTS(std::unique_ptr<Proposal> proposal);

  bool ContainsHightTS(int64_t global_sync);
  std::unique_ptr< TimeStampSync> GetHightSyncMsg();
  int SlotLen();
  std::unique_ptr<Proposal>GetSlotMsg(int slot);

  int GetTimeStamp();
  void UpateTimeStamp();
  int IncTimeStamp();

  void DoneOne();
  bool Ready();
  void RunOne();

 private:
  int32_t id_;
  int f_;
  int round_;
  int limit_count_;
  std::map<std::pair<int,int>, std::unique_ptr<Proposal> > data_;
  std::mutex txn_mutex_, local_mutex_, ts_mutex_, local_ts_mutex_;
  int seq_ = 0;
  std::vector<int64_t> highTS_;
  std::vector<std::unique_ptr<Proposal>> highTSMsgs_;
  std::map<int, std::vector<std::unique_ptr<Proposal> > > localSequencedSet_;
  int local_ts_;
  int p_num_ = 0;
  int n_;
};

}  // namespace tusk
}  // namespace resdb
