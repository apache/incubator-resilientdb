#pragma once

#include <functional>
#include <google/protobuf/message.h>
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

  inline
  void SetSingleCallFunc(SingleCallFuncType single_call) { single_call_ = single_call; }
  
  inline 
  void SetBroadcastCallFunc(BroadcastCallFuncType broadcast_call) { broadcast_call_ = broadcast_call; }

  inline
  void SetCommitFunc(CommitFuncType commit_func) { commit_ = commit_func; }

  inline 
  void SetSignatureVerifier(SignatureVerifier* verifier) { verifier_ = verifier;}

  protected:
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
};

}  // namespace protocol
}  // namespace resdb
