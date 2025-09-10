#pragma once

#include <functional>
#include <google/protobuf/message.h>
#include <random>
#include "common/crypto/signature_verifier.h"

namespace resdb {
namespace common {

class ProtocolBase {
 public:
  typedef std::function<int(int, const google::protobuf::Message& msg, int)> SingleCallFuncType;
  typedef std::function<int(int, const google::protobuf::Message& msg)> BroadcastCallFuncType;
  typedef std::function<int(const google::protobuf::Message& msg)> CommitFuncType;

  ProtocolBase(
      int id, 
    int f,
    int total_num,
      SingleCallFuncType single_call,
      BroadcastCallFuncType broadcast_call,
      CommitFuncType commit
      );

  ProtocolBase( int id, int f, int total_num);


  virtual ~ProtocolBase();

  void Stop();

  void SetNetworkDelayGenerator(int32_t network_delay_num, double mean_network_delay);

  inline
  void SetSingleCallFunc(SingleCallFuncType single_call) { single_call_ = single_call; }
  
  inline 
  void SetBroadcastCallFunc(BroadcastCallFuncType broadcast_call) { broadcast_call_ = broadcast_call; }

  inline
  void SetCommitFunc(CommitFuncType commit_func) { commit_ = commit_func; }

  inline 
  void SetSignatureVerifier(SignatureVerifier* verifier) { verifier_ = verifier;}

  inline
  uint64_t GetRandomDelay() { 
    // return (uint64_t)(dist_(gen_));
    return (uint64_t)(mean_network_delay_);
  }

  protected:
    bool IsSlowReplica(int node_id);
    int SendMessage(int msg_type, const google::protobuf::Message& msg, int node_id);
    int Broadcast(int msg_type, const google::protobuf::Message& msg);
    int Commit(const google::protobuf::Message& msg);

    bool IsStop();

 protected:
  int id_;
  int f_;
  int total_num_;
  std::function<int(int, const google::protobuf::Message& msg, int)> single_call_;
  std::function<int(int, const google::protobuf::Message& msg)> broadcast_call_;
  std::function<int(const google::protobuf::Message& msg)> commit_;
  std::atomic<bool> stop_;

  SignatureVerifier* verifier_;

  int32_t network_delay_num_; 
  double mean_network_delay_;  // Mean
  double sigma_;  // Standard deviation
  std::random_device rd_;  // Seed
  std::mt19937 gen_; // Mersenne Twister engine
  std::normal_distribution<> dist_;
};

}  // namespace protocol
}  // namespace resdb
