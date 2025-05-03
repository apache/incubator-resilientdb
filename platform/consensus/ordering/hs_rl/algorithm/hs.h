#pragma once

#include <thread>
#include <queue>

#include "platform/statistic/stats.h"
#include "platform/common/queue/lock_free_queue.h"
#include "platform/consensus/ordering/hs_rl/algorithm/proposal_manager.h"
#include "platform/consensus/ordering/hs_rl/algorithm/graph.h"
#include "platform/consensus/ordering/hs_rl/proto/proposal.pb.h"
#include "platform/consensus/ordering/common/algorithm/protocol_base.h"

namespace resdb {
namespace hs_rl {

class HotStuff: public common::ProtocolBase {
 public:
  HotStuff(int id, int f, int total_num, SignatureVerifier* verifier,
    common::ProtocolBase::SingleCallFuncType single_call,  
    common::ProtocolBase :: BroadcastCallFuncType broadcast_call,
    std::function<void(std::vector<Transaction*>& txns)> commit
  );
  ~HotStuff();

  //  recv txn -> send block with links -> rec block ack -> send block with certs
  bool ReceiveLocalOrdering(std::unique_ptr<Proposal> proposal);
  bool ReceiveTransaction(std::unique_ptr<Transaction> txn);
  bool ReceiveProposal(std::unique_ptr<Proposal> proposal);
  bool ReceiveCertificate(std::unique_ptr<Certificate> cert);

  std::unique_ptr<Transaction> FetchData(const std::string& hash);


  private:
    bool Ready();
    void StartNewRound();
    void AsyncSend();
    void AsyncSendLocalOrdering();
    void AsyncCommit();

    std::unique_ptr<Certificate> GenerateCertificate(const Proposal& proposal);

    int CurrentLeader(int view);
    int NextLeader(int view);
    bool IsLeader(int view);

    void CommitProposal(std::unique_ptr<Proposal> p);

 private:
  LockFreeQueue<Transaction> txns_;
  LockFreeQueue<Proposal> commit_q_;

  std::mutex mutex_, n_mutex_, pmutex_[1024], txn_mutex_;
  //std::mutex mutex_, n_mutex_;
  std::condition_variable vote_cv_;
  std::unique_ptr<ProposalManager> proposal_manager_;
  bool has_sent_;
  int execute_id_;
  SignatureVerifier * verifier_;
  common::ProtocolBase::SingleCallFuncType single_call_;
  common::ProtocolBase::BroadcastCallFuncType broadcast_call_;
  std::function<void(std::vector<Transaction*>& txns)> commit_;

  Stats* global_stats_;
  std::thread send_thread_, commit_thread_, lo_thread_;

  int batch_size_;
  //[view][hash][signer][cert]
  //std::map<std::string, std::map<int, std::unique_ptr<Certificate>> >  receive_[1024];
  std::map<int,  std::map<std::string, std::map<int, std::unique_ptr<Certificate>> > > receive_;
  std::set<std::pair<int,int> > committed_;

  std::map<std::pair<int, int>, std::unique_ptr<Transaction> > data_;
  std::map<std::pair<int,int>, std::set<int> > commit_proposers_;
  std::map<std::pair<int,int64_t>,int> commit_proposers_idx_;
  int idx_ = 0;
  std::map<std::pair<int,int>, std::set<int>> edge_counts_;
  std::map<int, std::set<int> > pending_proposals_;
  std::map<std::pair<int,int>, std::string> proposals_data_;
  std::map<int, std::pair<int,int> > proposals_key_;
  std::queue<std::unique_ptr<Graph> > g_list_;
  std::map<int, Graph*> id_g_;

std::map< std::pair<std::pair<int,int>, std::pair<int,int> > , int > count_edge_;
};

}  // namespace tusk
}  // namespace resdb
