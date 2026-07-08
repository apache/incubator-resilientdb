#pragma once

#include <condition_variable>
#include <list>
#include <set>
#include <string>
#include <vector>

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
  bool AddBlock(std::unique_ptr<Block> block);
  void AddLocalBlock(std::unique_ptr<Block> block);
  const Block* GetLocalBlock(int64_t block_id);
  Block* GetBlock(int sender, int64_t block_id);
  Block* GetLocalCandidateBlock(int64_t block_id);
  // Thread-safe deep copy. Returns true and writes into *out when the block
  // is found. Holds the internal mutex for the duration of the copy so the
  // caller is safe from concurrent mutations of pending_blocks_ /
  // blocks_candidates_.
  bool CopyBlock(int sender, int64_t block_id, Block* out);
  bool CopyLocalCandidateBlock(int64_t block_id, Block* out);
  int64_t GetCurrentBlockId();
  int64_t GetCertifiedBlockHeight(int sender);
  void SetExecutedBlockHeight(int sender, int64_t block_id);

  void BlockReady(const std::map<int, SignInfo>& sign_info, int64_t local_id);

  SignInfo SignBlock(const Block& block);
  bool VerifyBlock(const Block& block);

  bool ReadyView(int slot);
  int GetCurrentView();
  void IncreaseView();
  void SetCurrentView(int slot);
  void UpdateView(int sender, int64_t block_id);

  std::pair<int, std::map<int, int64_t>> GetCut(int slot);
  std::unique_ptr<Proposal> GenerateProposal(int slot, const std::map<int, int64_t>& blocks);
  std::string GetProposalHash(const Proposal& proposal);
  bool VerifyProposal(const Proposal& proposal);
  bool VerifyQC(const QC& qc);
  QC GetHighQC();
  QC GetLockedQC();
  void AddQC(std::unique_ptr<QC> qc);
  void UpdateLockedQC(const QC& qc);
  std::unique_ptr<Proposal> AddChainProposal(std::unique_ptr<Proposal> p);
  bool HasChainProposal(const std::string& hash);
  std::vector<Proposal> GetProposalChain(const std::string& hash, int limit);
  std::vector<Proposal> GetRecentProposals(int limit);
  std::map<int, int64_t> GetKnownBlockHeights();
  int64_t GetKnownBlockHeight(int sender);
  void GarbageCollect(int committed_slot);

  std::unique_ptr<Proposal> GetProposalData(int slot);
  void AddProposalData(std::unique_ptr<Proposal> p);


 private:
  void UpdateLastSign(Block * block);
  bool SafeNode(const Proposal& proposal);
  bool ExtendsLockedBlock(const Proposal& proposal);
  const Proposal* GetProposalByHash(const std::string& hash);
  std::unique_ptr<Proposal> TryCommitChainLocked(const std::string& hash);

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
  std::map<std::string, std::unique_ptr<Proposal> > chain_proposals_;
  std::map<std::string, std::set<std::string>> proposal_children_;
  std::set<std::string> committed_proposals_;
  QC high_qc_;
  QC locked_qc_;
  std::map<int, int64_t> certified_height_;
  std::map<int, int64_t> executed_height_;
  std::map<int, std::map<int, std::unique_ptr<Block> > > fur_;
};

}  // namespace autobahn
}  // namespace resdb
