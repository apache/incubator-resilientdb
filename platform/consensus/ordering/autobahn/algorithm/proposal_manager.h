#pragma once

#include <condition_variable>
#include <list>

#include "platform/consensus/ordering/autobahn/algorithm/proposal_graph.h"
#include "platform/consensus/ordering/autobahn/proto/proposal.pb.h"
#include "platform/statistic/stats.h"
#include "common/crypto/signature_verifier.h"

namespace resdb {
namespace autobahn {

class ProposalManager {
 public:
  ProposalManager(int32_t id, int total_num, int f, SignatureVerifier* verifier);

  void MakeBlock(
      std::vector<std::unique_ptr<Transaction>>& txn);
  void AddBlock(std::unique_ptr<Block> block);
  void AddLocalBlock(std::unique_ptr<Block> block);
  const Block* GetLocalBlock(int64_t block_id);
  Block* GetBlock(int sender, int64_t block_id);
  int64_t GetCurrentBlockId();

  void BlockReady(const std::map<int, SignInfo>& sign_info, int64_t local_id);

  SignInfo SignBlock(const Block& block);
  bool VerifyBlock(const Block& block);

  bool ReadyView(int slot);
  int GetCurrentView();
  void IncreaseView();
  void UpdateView(int sender, int64_t block_id);

  std::pair<int, std::map<int, int64_t>> GetCut();
  std::unique_ptr<Proposal> GenerateProposal(int slot, const std::map<int, int64_t>& blocks);

  std::unique_ptr<Proposal> GetProposalData(int slot);
  void AddProposalData(std::unique_ptr<Proposal> p);


 private:
  void UpdateLastSign(Block * block);

 private:
  int32_t id_;
  int64_t local_block_id_ = 1;

  std::map<int64_t, std::unique_ptr<Block>> pending_blocks_[512];
  std::mutex mutex_, slot_mutex_, p_mutex_;
  std::map<int, std::unique_ptr<Block>> blocks_candidates_;

  std::map<int, std::pair<int, int64_t>> slot_state_;
  std::map<int,int> new_blocks_;

  //std::mutex t_mutex_;
  //std::map<std::string, std::unique_ptr<Proposal>> local_proposal_;
  //Stats* global_stats_;
  int total_num_;
  int f_;
  int64_t current_height_;
  int current_slot_;

  SignatureVerifier* verifier_;

  std::map<int, std::unique_ptr<Proposal> > pending_proposals_;
};

}  // namespace autobahn
}  // namespace resdb
