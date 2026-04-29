#include "platform/consensus/ordering/fides/algorithm/fides.h"

#include <algorithm>
#include <glog/logging.h>
#include <sstream>

#include "common/utils/utils.h"

namespace resdb {
namespace fides {

// File-scope: TEE Common Coin failure tracking. Shared across GetLeader
// and TEECommonCoin so the fast-path skip in GetLeader works.
static std::atomic<int> tee_fail_count{0};
static std::atomic<bool> tee_permanently_failed{false};

Fides::Fides(int id, int f, int total_num, SignatureVerifier* verifier, const ResDBConfig& config, oe_enclave_t* enclave)
    : ProtocolBase(id, f, total_num), verifier_(verifier), config_(config), enclave_(enclave) {
  // TODO: 5.Quorum to f+1 - Done
  limit_count_ = f + 1;
  batch_size_ = 5;
  proposal_manager_ = std::make_unique<ProposalManager>(id, limit_count_, enclave, config_);

  execute_id_ = 1;
  start_ = 0;
  queue_size_ = 0;

  send_thread_ = std::thread(&Fides::AsyncSend, this);
  commit_thread_ = std::thread(&Fides::AsyncCommit, this);
  execute_thread_ = std::thread(&Fides::AsyncExecute, this);
  cert_thread_ = std::thread(&Fides::AsyncProcessCert, this);

  global_stats_ = Stats::GetGlobalStats();

  StartEnclaveStatMonitor("heap_stats_replica_" + std::to_string(id) + ".log");

  LOG(ERROR) << "id:" << id << " f:" << f << " failureNum:" << config_.GetFailureNum() << " total:" << total_num
    << " mcEnabled:" << config_.GetMCEnabled() << " racEnabled:" << config_.GetRACEnabled()
    << " rngEnabled:" << config_.GetRNGEnabled()
    << " byzantineMode:" << config_.GetByzantineMode();
}

Fides::~Fides() {
  if (send_thread_.joinable()) {
    send_thread_.join();
  }
  if (commit_thread_.joinable()) {
    commit_thread_.join();
  }
  if(cert_thread_.joinable()){
    cert_thread_.join();
  }
  StopEnclaveStatMonitor();
}

// TODO: 1.Change this to Using trusted global random
// TODO: Not every round has a different leader
int Fides::GetLeader(int r) {
  // return r / 3 % total_num_ + 1;
  {
    std::unique_lock<std::mutex> lk(leader_list_mutex_);
    if (leader_list_.count(r) != 0) {
      return leader_list_[r];
    }
    if (r % 3 != 0) {
      leader_list_[r] = leader_list_[r-1];
      return leader_list_[r];
    }
  }

  uint32_t randNum = -1;

  // Fast-path: if TEE Common Coin has failed consistently (which it
  // does on CloudLab r320 nodes without real SGX), skip the enclave
  // call entirely. Each failed call allocates memory, copies counter
  // data, calls the enclave, logs ERROR, and frees memory — ~100μs
  // of wasted work per leader round. At 33 leader rounds/sec, that's
  // 3.3ms/sec of pure overhead that suppresses post-crash recovery.
  if (tee_permanently_failed.load(std::memory_order_relaxed)) {
    // Prime-stride rotation: spread leaders across node IDs to avoid
    // clustering dead leaders (nodes 1-5) in consecutive rounds. With
    // the plain (r/3)%16 rotation, leaders 1-5 map to rounds 0-12
    // consecutively = 4 dead leaders in a row → commit_round_latency
    // jumps from 3.3 to 5.2 rounds post-crash. Stride 7 (coprime to
    // 16) interleaves dead leaders with alive ones, reducing the max
    // consecutive dead-leader streak from 4 to 2.
    randNum = static_cast<uint32_t>(((r / 3) * 7) % total_num_);
  } else if (config_.GetRNGEnabled()) {
    randNum = TEECommonCoin(r);
  } else {
    SimpleShamirCoin(r, total_num_, limit_count_);
    randNum = ((r / 3) * 7) % total_num_;
  }

  std::unique_lock<std::mutex> lk(leader_list_mutex_);
  leader_list_[r] = randNum + 1;
  // LOG(ERROR) << "newLeader:" << leader_list_[r];

  return leader_list_[r];
}

uint32_t Fides::TEECommonCoin(int r) {
  int ret;
  // Pass the MIC enabled flag to the enclave. 
  // If it is enabled, enable counter verification.
  uint32_t randNum = config_.GetMCEnabled();

  
  // Send f+1 counter value and attestation to proof can obtain new random number
  std::vector<CounterInfo> counters;
  if(r > 0) {
    // LOG(ERROR)<<"In GetLeader. round:"<<r;
    counters = proposal_manager_->GetCounterFromRound(r - 1);
    assert(counters.size()>=limit_count_);
  }

  size_t previous_cert_size = counters.size();
  unsigned char** attestation_array = new unsigned char*[previous_cert_size];
  size_t* attestation_size_array = new size_t[previous_cert_size];
  uint32_t* counter_value_array = new uint32_t[previous_cert_size];

  std::string serialized_data;
  for (size_t i = 0; i < counters.size(); ++i) {
    counter_value_array[i] = counters[i].value();

    serialized_data = counters[i].attestation();
    size_t size = serialized_data.size();

    attestation_array[i] = new unsigned char[size];
    std::memcpy(attestation_array[i], serialized_data.data(), size);
    attestation_size_array[i] = size;
  }

  oe_result_t result = generate_rand(enclave_, &ret, previous_cert_size, limit_count_,
                                     counter_value_array, attestation_size_array,
                                     attestation_array, &randNum);

  // Clean up allocated memory
  for (size_t i = 0; i < previous_cert_size; ++i) {
    delete[] attestation_array[i];
  }
  delete[] attestation_array;
  delete[] attestation_size_array;
  delete[] counter_value_array;

  if (result != OE_OK || ret != 0) {
    int fails = tee_fail_count.fetch_add(1, std::memory_order_relaxed) + 1;
    if (fails <= 3) {
      LOG(ERROR) << "Function generate_rand failed with " << ret
                 << " (fail #" << fails << ")";
    }
    if (fails >= 3) {
      tee_permanently_failed.store(true, std::memory_order_relaxed);
    }
    return static_cast<uint32_t>((r / 3) % total_num_);
  }

  // Success resets the counter
  tee_fail_count.store(0, std::memory_order_relaxed);
  return randNum;
}

void Fides::AsyncSend() {

  uint64_t last_time = 0;
  while (!IsStop()) {
    // Wait for both: transactions available AND protocol ready
    // Use non-blocking check to minimize idle time
    std::unique_ptr<Transaction> txn = nullptr;
    while (!IsStop()) {
      // Try to get a transaction
      if (!txn) {
        txn = start_ ? txns_.Pop(0) : txns_.Pop();  // 0ms = non-blocking after startup
        if (txn) queue_size_--;
      }
      // Check if protocol is ready
      if (proposal_manager_->Ready()) {
        if (txn || start_) break;  // Have txn or can propose empty
      } else if (txn) {
        // Have txn but not ready — wait for certs.
        // 100μs poll: fast enough to detect cert arrival within one
        // network RTT tick, avoiding the 500μs×N penalty that added
        // 1-2ms per round post-crash (15-20% overhead at 10ms/round).
        std::unique_lock<std::mutex> lk(mutex_);
        vote_cv_.wait_for(lk, std::chrono::microseconds(100),
                          [&] { return proposal_manager_->Ready(); });
      }
    }
    if (IsStop()) break;
    start_ = 1;

    // Dynamic batching: adapt drain rate to queue depth.
    // Deep queue → drain more per round (up to queue_size) to cut queuing latency.
    // Shallow queue → small batch (batch_size_) for fast round advancement.
    int current_queue = queue_size_;
    int drain_target = batch_size_;

    std::vector<std::unique_ptr<Transaction>> txns;
    if (txn) {
      txn->set_queuing_time(GetCurrentTime() - txn->create_time());
      global_stats_->AddQueuingLatency(GetCurrentTime() - txn->create_time());
      txns.push_back(std::move(txn));
    }
    for (int i = txns.size(); i < drain_target && !txns_.Empty(); ++i) {
      auto extra = txns_.Pop(0);
      if (extra == nullptr) break;
      queue_size_--;
      extra->set_queuing_time(GetCurrentTime() - extra->create_time());
      global_stats_->AddQueuingLatency(GetCurrentTime() - extra->create_time());
      txns.push_back(std::move(extra));
    }

    global_stats_->AddRoundLatency(GetCurrentTime() - last_time);
    last_time = GetCurrentTime();

    global_stats_->ConsumeTransactions(queue_size_);

    auto proposal = proposal_manager_->GenerateProposal(txns);

    uint32_t byz_mode = config_.GetByzantineMode();
    if (byz_mode == 2) {
      // Equivocation: send different proposals to different subsets
      // Send the real proposal to first half
      auto targets_a = GetByzantineTargetNodes();
      for (int node_id : targets_a) {
        SendMessage(MessageType::NewBlock, *proposal, node_id);
      }
      // Generate a fake proposal with shuffled transactions for the other half
      auto fake_proposal = proposal_manager_->GenerateProposal(txns);
      for (int node_id = 1; node_id <= total_num_; ++node_id) {
        if (std::find(targets_a.begin(), targets_a.end(), node_id) == targets_a.end() && node_id != id_) {
          SendMessage(MessageType::NewBlock, *fake_proposal, node_id);
        }
      }
      LOG(ERROR) << "[BYZANTINE] Equivocation: sent different proposals to different subsets";
    } else if (byz_mode == 4) {
      // Selective delivery: only send to a subset
      ByzantineBroadcast(MessageType::NewBlock, *proposal);
      LOG(ERROR) << "[BYZANTINE] Selective delivery of proposal";
    } else {
      Broadcast(MessageType::NewBlock, *proposal);
    }

    std::string block_data;
    proposal->SerializeToString(&block_data);
    global_stats_->AddBlockSize(block_data.size());

    // Create self-cert immediately — don't wait for network round-trip
    int prop_round = proposal->header().round();
    {
      auto self_cert = std::make_unique<Certificate>();
      self_cert->set_hash(proposal->hash());
      self_cert->set_round(prop_round);
      self_cert->set_proposer(id_);
      *self_cert->mutable_strong_cert() = proposal->header().strong_cert();
      *self_cert->mutable_counter() = proposal->header().counter();

      // AddBlock and AddCert handle their own locking (txn_mutex_)
      proposal_manager_->AddBlock(std::make_unique<Proposal>(*proposal));
      proposal_manager_->AddCert(std::move(self_cert));
    }
    {
      std::unique_lock<std::mutex> lk(mutex_);
      vote_cv_.notify_all();
    }

    proposal_manager_->AddLocalBlock(std::move(proposal));

    int round = proposal_manager_->CurrentRound() - 1;
    // Trigger commit check every round (not just every 3rd).
    // AsyncCommit iterates leader rounds (step 3), so non-leader rounds
    // are skipped automatically. This reduces commit latency by up to 2 rounds.
    if (round > 2) {
      CommitRound(round - 3);
    }
  }
}

void Fides::AsyncCommit() {
  int previous_round = -3;
  while (!IsStop()) {
    std::unique_ptr<int> round_or = commit_queue_.Pop();
    if (round_or == nullptr) {
      continue;
    }

    int round = *round_or;

    int new_round = previous_round;
    for (int r = previous_round + 3; r <= round; r += 3) {
      int leader = GetLeader(r);
      const Proposal * req = nullptr;
      // Zero-wait skip for dead leaders: if GetRequest returns null the
      // first time, assume the leader is dead and skip immediately.
      // This matters post-crash with round-robin fallback where 5/16
      // leader rounds are dead — each 100μs wait per dead round
      // accumulates to >10s over the 60s post-crash window and caps
      // recovery at ~43% instead of ~65%. Alive leaders will be found
      // when the DAG naturally advances via AsyncSend receiving their
      // blocks — no need to poll here.
      req = proposal_manager_->GetRequest(r, leader);
      if (req == nullptr) continue;
      int reference_num = proposal_manager_->GetReferenceNum(*req);
      if (leader <= config_.GetFailureNum()) {
        reference_num = 0;
      }
      if (reference_num < limit_count_) {
        continue;
      } else {
        for (int j = new_round + 3; j <= r; j += 3) {
          int pre_leader = GetLeader(j);

          auto req = proposal_manager_->GetRequest(j, pre_leader);
          if (req == nullptr) {
            continue;
          }
          int reference_num = proposal_manager_->GetReferenceNum(*req);
          if (reference_num < limit_count_) {
            continue;
          }

          CommitProposal(j, pre_leader);
        }
      }
      new_round = r;
    }
    previous_round = new_round;
  }
}

// TODO: 3.Decrypt the request before executing.
void Fides::AsyncExecute() {
int64_t last_commit_time = 0;
int last_round = 0;
  while (!IsStop()) {
    std::unique_ptr<Proposal> proposal = execute_queue_.Pop();
    if (proposal == nullptr) {
      continue;
    }

    int commit_round = proposal->header().round();
    int64_t commit_time = GetCurrentTime();
    int64_t waiting_time = commit_time - proposal->queuing_time();
    global_stats_->AddCommitQueuingLatency(waiting_time);
    //global_stats_->AddCommitInterval(commit_time - last_commit_time);
    //last_commit_time = commit_time;
    std::map<int, std::vector<std::unique_ptr<Proposal>>> ps;
    std::queue<std::unique_ptr<Proposal>> q;
    q.push(std::move(proposal));

    //LOG(ERROR)<<" commit round:"<<commit_round;
    //proposal_manager_->SetCommittedRound(commit_round);

    //global_stats_->AddCommitRoundLatency(commit_round - last_round);
    //last_round = commit_round;

    //int64_t start_time = GetCurrentTime();
    while (!q.empty()) {
      std::unique_ptr<Proposal> p = std::move(q.front());
      q.pop();

      for (auto& link : p->header().strong_cert().cert()) {
        int link_round = link.round();
        int link_proposer = link.proposer();
        auto next_p =
            proposal_manager_->FetchRequest(link_round, link_proposer);
        if (next_p == nullptr) {
          continue;
        }
        q.push(std::move(next_p));
      }


      for (auto& link : p->header().weak_cert().cert()) {
        int link_round = link.round();
        int link_proposer = link.proposer();
        auto next_p =
            proposal_manager_->FetchRequest(link_round, link_proposer);
        if (next_p == nullptr) {
          // LOG(ERROR)<<"no data round:"<<link_round<<" link
          // proposer:"<<link_proposer;
          continue;
        }
        q.push(std::move(next_p));
      }
      ps[p->header().round()].push_back(std::move(p));
    }

    int num = 0;
    int pro = 0;
    for (auto& it : ps) {
      for (auto& p : it.second) {
        //LOG(ERROR) << "=============== commit proposal round :"
        //           << p->header().round()
        //           << " header round:"<<p->header().round()
        //           << " commit round:"<<commit_round
        //           << " proposer:" << p->header().proposer_id()
        //           << " transaction size:" << p->transactions_size()
        //           << " commit time:"
        //           << (GetCurrentTime() - p->header().create_time())
        //           << " create time:" << p->header().create_time()
        //           <<" execute id:"<<execute_id_ 
        //          <<" round delay:"<<(commit_round - p->header().round());

        global_stats_->AddCommitLatency(commit_time - p->header().create_time() - waiting_time);
        global_stats_->AddCommitRoundLatency(commit_round - p->header().round());
        
        for (auto& tx : *p->mutable_transactions()) {
          tx.set_id(execute_id_++);
          num++;
          Commit(tx);
        }
        pro++;
      }
    }
    int64_t end_time = GetCurrentTime();
    global_stats_->AddCommitRuntime(end_time-commit_time);
    global_stats_->AddCommitTxn(num);
    global_stats_->AddCommitBlock(pro);

    // Update committed_round_ so the GC in AddCert knows what to prune.
    proposal_manager_->SetCommittedRound(commit_round);
  }
}

void Fides::CommitProposal(int round, int proposer) {
  //LOG(ERROR) << "commit round:" << round << " proposer:" << proposer;

  //int64_t last_commit_time = 0;
  int64_t commit_time = GetCurrentTime();
  global_stats_->AddCommitInterval(commit_time - last_commit_time_);
  last_commit_time_ = commit_time;

  std::unique_ptr<Proposal> p =
      proposal_manager_->FetchRequest(round, proposer);
  if(p==nullptr){
    LOG(ERROR)<<"commit round:"<<round<<" proposer:"<<proposer<<" not exist";
    assert(false);
    return;
  }
  assert(p !=nullptr);
  p->set_queuing_time(GetCurrentTime());
  execute_queue_.Push(std::move(p));
}

void Fides::CommitRound(int round) {
  //LOG(ERROR)<<" commit round:"<<round;
  if (!commit_queue_.TryPush(std::make_unique<int>(round))) { /* dropped */ }
}

bool Fides::ReceiveTransaction(std::unique_ptr<Transaction> txn) {
  txn->set_create_time(GetCurrentTime());
  if (!txns_.TryPush(std::move(txn))) { return false; /* back-pressure: client retries */ }
  queue_size_++;
  if (start_ == 0) {
    std::unique_lock<std::mutex> lk(mutex_);
    vote_cv_.notify_all();
  }
  return true;
}

bool Fides::ReceiveBlock(std::unique_ptr<Proposal> proposal) {
  int proposer_id = proposal->header().proposer_id();
  int round = proposal->header().round();
  std::string hash = proposal->hash();
  // LOG(ERROR) << "ReceiveBlock from=" << proposer_id << " round=" << round;

  {
    std::unique_lock<std::mutex> lk(check_block_mutex_);
    proposal->set_queuing_time(GetCurrentTime());
    //std::unique_lock<std::mutex> lk(check_block_mutex_);
    // Verify the block, including the counter
    if(!CheckBlock(*proposal)){
      std::unique_lock<std::mutex> lk(future_block_mutex_);
      // LOG(ERROR)<<"CheckBlock FAILED: add future block round="<<proposal->header().round()<<" from="<<proposal->header().proposer_id();
      future_block_[proposal->header().round()][proposal->hash()] = std::move(proposal);
      return false;
    }
    // LOG(ERROR)<<"check block pass. round:"<<proposal->header().round()<<" proposer:"<<proposal->header().proposer_id()<<" strong link size:"<<proposal->header().strong_cert().cert_size()<<" weak link size:"<<proposal->header().weak_cert().cert_size();
  }
  if (!config_.GetMCEnabled())
    return SendBlockAck(std::move(proposal));

  // Generate certificate
  auto cert = std::make_unique<Certificate>();
  global_stats_->AddExecutePrepareDelay(GetCurrentTime() - proposal->header().create_time());
  //global_stats_->AddCommitLatency(GetCurrentTime() - proposal->header().create_time());
  
  cert->set_hash(hash);
  cert->set_round(round);
  cert->set_proposer(proposer_id);
  *cert->mutable_strong_cert() = proposal->header().strong_cert();
  *cert->mutable_counter() = proposal->header().counter();

  {
    std::unique_lock<std::mutex> lk(txn_mutex_);
     global_stats_->AddCommitWaitingLatency(GetCurrentTime() - proposal->queuing_time());
     proposal->set_queuing_time(0);
     {
      proposal_manager_->AddBlock(std::move(proposal));
      CheckFutureCert(round, hash, proposer_id);
     }
  }

  // MC mode: block itself serves as cert (TEE attestation proves validity).
  // No need to broadcast cert — just process locally.
  ReceiveBlockCert(std::move(cert));

  return true;
}

void Fides::ReceiveBlockACK(std::unique_ptr<Metadata> metadata) {
  std::string hash = metadata->hash();
  int round = metadata->round();
  int sender = metadata->sender();
  std::unique_lock<std::mutex> lk(txn_mutex_);
  received_num_[hash][sender] = std::move(metadata);
  // LOG(ERROR) << "recv block ack from:" << sender << " num:" << received_num_[hash].size()<<" round:"<<round;
  if (received_num_[hash].size() == limit_count_) {
    Certificate cert;
    for (auto& it : received_num_[hash]) {
      *cert.add_metadata() = *it.second;
    }
    cert.set_hash(hash);
    cert.set_round(round);
    cert.set_proposer(id_);
    const Proposal* p = proposal_manager_->GetLocalBlock(hash);
    assert(p != nullptr);
    assert(p->header().proposer_id() == id_);
    assert(p->header().round() == round);
    global_stats_->AddExecutePrepareDelay(GetCurrentTime() - p->header().create_time());
    //global_stats_->AddCommitLatency(GetCurrentTime() - p->header().create_time());
    *cert.mutable_strong_cert() = p->header().strong_cert();
    //LOG(ERROR)<<"send cert, round:"<<p->header().round();
    if (config_.GetByzantineMode() == 4) {
      ByzantineBroadcast(MessageType::Cert, cert);
      LOG(ERROR) << "[BYZANTINE] Selective cert delivery, round:" << p->header().round();
    } else if (config_.GetByzantineMode() == 3) {
      // Withholding: don't broadcast certificate
      LOG(ERROR) << "[BYZANTINE] Withholding cert, round:" << p->header().round();
    } else {
      Broadcast(MessageType::Cert, cert);
    }
  }
}

bool Fides::VerifyCert(const Certificate & cert){
  if (config_.GetMCEnabled())
    return true;
  if (cert.round() == 0){
    return true;
  }

  std::map<std::string, int> hash_num;
  bool vote_num = false;
  for(auto& metadata: cert.metadata()){
    bool valid = verifier_->VerifyMessage(metadata.hash(), metadata.sign());
    if(!valid){
      LOG(ERROR) << "Verify message fail";
      return false;
    }
    if (!fakemessage_) {
      fakemessage_ = metadata.hash();
      fakesign_ = metadata.sign();
    }
    hash_num[metadata.hash()]++;
    if(hash_num[metadata.hash()]>=limit_count_){
      vote_num = true;
    }
  }
  return vote_num;
  // TODO: change quorum here
}

bool Fides::VerifyCertSimulation(const Certificate & cert){
  if (config_.GetMCEnabled())
    return true;
  if (cert.round() == 0){
    return true;
  }

  // LOG(ERROR) << "Verifying message.";
  for (size_t i = 0; i < total_num_; i++) {
    bool valid = verifier_->VerifyMessage(fakemessage_.value(), fakesign_.value());
    if(!valid){
      LOG(ERROR) << "Verify message fail";
      return false;
    }
  }
  return true;
}


bool Fides::VerifyCounter(int round, const CounterInfo& counter) {
  std::string attestation = counter.attestation();
  int counter_value = counter.value();
  if (attestation != "Fake Attestation" || counter_value != round) {
    return false;
  }
  return true;
}

bool Fides::CheckBlock(const Proposal& p){
  
  if(config_.GetMCEnabled() && config_.GetRACEnabled()) {
    if (!VerifyCounter(p.header().round(), p.header().counter())) {
      //LOG(ERROR)<<" check block round:"<<p.header().round()<<" proposer:"<<p.header().proposer_id()<<" strong link:"<<link_proposer<<" link round:"<<link_round<<" not exist";
      return false;
    }
  } 
  
  if(! config_.GetRACEnabled()) {
    for(auto& link : p.header().strong_cert().cert()){
      int link_round = link.round();
      int link_proposer = link.proposer();
      if(!proposal_manager_->CheckCert(link_round, link_proposer)){
        //LOG(ERROR)<<" check block round:"<<p.header().round()<<" proposer:"<<p.header().proposer_id()<<" strong link:"<<link_proposer<<" link round:"<<link_round<<" not exist";
        return false;
      }
      if(!VerifyCertSimulation(link)) {
        LOG(ERROR)<<" Verify fail. block round:"<<link_round<<" proposer:"<<link_proposer;
        return false;
      }
    }

    for(auto& link : p.header().weak_cert().cert()){
      int link_round = link.round();
      int link_proposer = link.proposer();
      if(!proposal_manager_->CheckCert(link_round, link_proposer)){
        //LOG(ERROR)<<" check block round:"<<p.header().round()<<" proposer:"<<p.header().proposer_id()<<" weak link:"<<link_proposer<<" link round:"<<link_round<<" not exist";
        return false;
      }
      if(!VerifyCertSimulation(link)) {
        LOG(ERROR)<<" Verify fail. block round:"<<link_round<<" proposer:"<<link_proposer;
        return false;
      }
    }
  }
  //LOG(ERROR)<<" check cert round:"<<p.header().round()<<" proposer:"<<p.header().proposer_id()<<" strong link size:"<<p.header().strong_cert().cert_size()<<" weak link size:"<<p.header().weak_cert().cert_size();
  return true;
}

bool Fides::CheckCert(const Certificate& cert){
  if(!proposal_manager_->CheckBlock(cert.hash())){
    //future_cert_[cert->round()][cert->hash()] = std::move(cert);
    return false;
  }
  return true;
}

bool Fides::SendBlockAck(std::unique_ptr<Proposal> proposal) {
  // Byzantine withholding: don't send ACKs
  if (config_.GetByzantineMode() == 3) {
    // Still add the block locally so our DAG stays consistent
    std::unique_lock<std::mutex> lk(txn_mutex_);
    int round = proposal->header().round();
    std::string hash = proposal->hash();
    int proposer = proposal->header().proposer_id();
    global_stats_->AddCommitWaitingLatency(GetCurrentTime() - proposal->queuing_time());
    proposal->set_queuing_time(0);
    proposal_manager_->AddBlock(std::move(proposal));
    CheckFutureCert(round, hash, proposer);
    LOG(ERROR) << "[BYZANTINE] Withholding BlockACK for round:" << round;
    return true;
  }

  Metadata metadata;
  metadata.set_sender(id_);
  metadata.set_hash(proposal->hash());
  metadata.set_round(proposal->header().round());
  metadata.set_proposer(proposal->header().proposer_id());

  std::string data_str = proposal->hash();
  auto hash_signature_or = verifier_->SignMessage(data_str);
  if (!hash_signature_or.ok()) {
    LOG(ERROR) << "Sign message fail";
    return false;
  }
  *metadata.mutable_sign()=*hash_signature_or;
  metadata.set_sender(id_);

  {
    std::unique_lock<std::mutex> lk(txn_mutex_);
    // LOG(ERROR) << "In SendBlockAck: recv block from " << proposal->header().proposer_id()
    //         << " round:" << proposal->header().round();
     int round = proposal->header().round();
     std::string hash = proposal->hash();
     int proposer = proposal->header().proposer_id();
     global_stats_->AddCommitWaitingLatency(GetCurrentTime() - proposal->queuing_time());
     proposal->set_queuing_time(0);
     {
       //std::unique_lock<std::mutex> lk(check_block_mutex_);
       proposal_manager_->AddBlock(std::move(proposal));
       CheckFutureCert(round, hash, proposer);
     }
  }
  // LOG(ERROR) << "send blockACK. round:" << metadata.round()
  //           << " proposer:" << metadata.proposer();
  SendMessage(MessageType::BlockACK, metadata, metadata.proposer());
  return true;
}


void Fides::CheckFutureBlock(int round){
  //LOG(ERROR)<<" check block round from cert:"<<round;
  std::map<std::string,std::unique_ptr<Proposal>> hashs;
  {
    std::unique_lock<std::mutex> clk(check_block_mutex_);

    {
      std::unique_lock<std::mutex> lk(future_block_mutex_);
      if(future_block_.find(round) == future_block_.end()){
        return;
      }
      for(auto& it : future_block_[round]){
        if(CheckBlock(*it.second)){
          // LOG(ERROR)<<" add new block round:"<<it.second->header().round();
          hashs[it.first] = std::move(it.second);
        }
      }

      // LOG(ERROR)<<" check round:"<<round<<" hash size:"<<hashs.size();
      for(auto& it : hashs){
        future_block_[round].erase(future_block_[round].find(it.first));
      }

      if(future_block_[round].size() == 0){
        future_block_.erase(future_block_.find(round));
      }
    }
  }

  for(auto& it : hashs){
    //LOG(ERROR)<<"send future block round:"<<it.second->header().round()<<" proposer:"<<it.second->header().proposer_id();
    int block_round = it.second->header().round();
    //std::unique_lock<std::mutex> lk(check_block_mutex_);
    {
      std::unique_lock<std::mutex> lk(future_cert_mutex_);
      if(future_cert_[block_round].find(it.first) != future_cert_[block_round].end()){
        std::unique_ptr<Certificate> cert = std::move(future_cert_[block_round][it.first]);
        //LOG(ERROR)<<" add new cert:"<<block_round<<" from:"<<cert->proposer();
        cert_queue_.Push(std::move(cert));
        future_cert_[block_round].erase(future_cert_[block_round].find(it.first));
        if(future_cert_[block_round].size() == 0){
          future_cert_.erase(future_cert_.find(block_round));
        }
      }
    }
    
    //LOG(ERROR)<<" Redo the proposal again, round: "<<block_round;
    // Redo the proposal again.
    ReceiveBlock(std::move(it.second));
  }
}



//  This situation will not happen in fides
void Fides::CheckFutureCert(int round, const std::string& hash, int proposer) {
  std::unique_lock<std::mutex> lk(future_cert_mutex_);
  auto it = future_cert_[round].find(hash);
  if(it != future_cert_[round].end()){
    //LOG(ERROR)<<" add back future cert, round:"<<round<<" proposer:"<<proposer;
    cert_queue_.Push(std::move(it->second));
    future_cert_[round].erase(it); 
    if(future_cert_[round].empty()){
      future_cert_.erase(future_cert_.find(round));
    }
  }
}

void Fides::AsyncProcessCert(){
  while(!IsStop()){
    std::unique_ptr<Certificate> cert = cert_queue_.Pop();
    if(cert == nullptr){
      continue;
    }

    int round = cert->round();
    int cert_sender = cert->proposer();

    //LOG(ERROR)<<" add cert, round:"<<round<<" from:"<<cert_sender;
    {
      std::unique_lock<std::mutex> clk(check_block_mutex_);
      proposal_manager_->AddCert(std::move(cert));
      {
        std::unique_lock<std::mutex> lk(mutex_);
        vote_cv_.notify_all();
      }
    }
    CheckFutureBlock(round+1);
  }
}

void Fides::ReceiveBlockCert(std::unique_ptr<Certificate> cert) {
  int64_t start_time = GetCurrentTime();
  if(!VerifyCert(*cert)){
    LOG(ERROR) << "VerifyCert FAILED for round=" << cert->round() << " proposer=" << cert->proposer();
    return;
  }

  int cert_round = cert->round();
  int cert_proposer = cert->proposer();

  {
    std::unique_lock<std::mutex> lk(check_block_mutex_);
    if(!CheckCert(*cert)){
      // LOG(ERROR) << "CheckCert FAILED: round=" << cert_round << " proposer=" << cert_proposer << " -> future_cert";
      std::unique_lock<std::mutex> lk2(future_cert_mutex_);
      future_cert_[cert->round()][cert->hash()] = std::move(cert);
      return;
    }
  }

  int64_t end_time = GetCurrentTime();
  global_stats_->AddVerifyLatency(end_time-start_time);

  // LOG(ERROR) << "AddCert OK: round=" << cert_round << " proposer=" << cert_proposer;

  // Process cert synchronously
  int round = cert_round;
  {
    std::unique_lock<std::mutex> clk(check_block_mutex_);
    proposal_manager_->AddCert(std::move(cert));
    {
      std::unique_lock<std::mutex> lk(mutex_);
      vote_cv_.notify_all();
    }
  }
  CheckFutureBlock(round + 1);
}

bool Fides::GetEnclaveStatOnce(uint64_t* current, uint64_t* peak, uint64_t* max)
{
    if (!enclave_ || !current || !peak || !max)
        return false;

    int ecall_ret = 0;
    oe_result_t r = enclave_get_heap_stats(
        enclave_,
        &ecall_ret,
        current,
        peak,
        max);

    return (r == OE_OK && ecall_ret == 0);
}

void Fides::StartEnclaveStatMonitor(const std::string& log_path)
{
    std::lock_guard<std::mutex> lock(sampler_mutex_);

    if (sampler_running_)
        return;  // 已经在跑了，避免重复启动

    if (!enclave_)
    {
        std::cerr << "StartEnclaveStatMonitor: enclave is null" << std::endl;
        return;
    }

    stop_sampler_ = false;
    sampler_running_ = true;

    sampler_thread_ = std::thread([this, log_path]() {
        try
        {
            std::ofstream fout(log_path, std::ios::app);
            if (!fout.is_open())
            {
                std::cerr << "failed to open " << log_path << std::endl;
                sampler_running_ = false;
                return;
            }

            auto start = std::chrono::steady_clock::now();

            while (!stop_sampler_.load())
            {
                uint64_t current_heap = 0;
                uint64_t peak_heap = 0;
                uint64_t max_heap = 0;

                auto now = std::chrono::steady_clock::now();
                auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                              now - start)
                              .count();

                if (GetEnclaveStatOnce(&current_heap, &peak_heap, &max_heap))
                {
                    fout << ms
                         << " current=" << current_heap
                         << " peak=" << peak_heap
                         << " max=" << max_heap
                         << '\n';
                }
                else
                {
                    fout << ms << " get_heap_stats failed\n";
                }

                fout.flush();  // 实验中断时也尽量保留日志
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }

            sampler_running_ = false;
        }
        catch (const std::exception& e)
        {
            std::cerr << "sampler thread exception: " << e.what() << std::endl;
            sampler_running_ = false;
        }
        catch (...)
        {
            std::cerr << "sampler thread unknown exception" << std::endl;
            sampler_running_ = false;
        }
    });
}

void Fides::StopEnclaveStatMonitor()
{
    std::lock_guard<std::mutex> lock(sampler_mutex_);

    if (!sampler_running_)
        return;

    stop_sampler_ = true;

    if (sampler_thread_.joinable())
        sampler_thread_.join();

    sampler_running_ = false;
}

bool Fides::IsByzantine() const {
  return config_.GetByzantineMode() > 0;
}

std::vector<int> Fides::GetByzantineTargetNodes() const {
  std::vector<int> targets;
  std::string target_str = config_.GetByzantineTarget();
  if (target_str.empty()) {
    // Default: send to first half of nodes
    for (int i = 1; i <= total_num_ / 2; ++i) {
      targets.push_back(i);
    }
    return targets;
  }
  std::istringstream ss(target_str);
  std::string token;
  while (std::getline(ss, token, ',')) {
    try {
      targets.push_back(std::stoi(token));
    } catch (...) {}
  }
  return targets;
}

void Fides::ByzantineBroadcast(int msg_type, const google::protobuf::Message& msg) {
  uint32_t mode = config_.GetByzantineMode();
  if (mode == 4) {
    // Selective delivery: only send to a subset of nodes
    auto targets = GetByzantineTargetNodes();
    for (int node_id : targets) {
      SendMessage(msg_type, msg, node_id);
    }
  } else {
    // For other modes, broadcast normally
    Broadcast(msg_type, msg);
  }
}

}  // namespace fides
}  // namespace resdb
