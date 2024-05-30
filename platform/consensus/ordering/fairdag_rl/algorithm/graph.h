#pragma once

#include <map>
#include "platform/consensus/ordering/fairdag_rl/proto/proposal.pb.h"

namespace resdb {
namespace fairdag_rl {

class Graph {
public:
  void AddTxn(const Transaction& a, const Transaction& b);
  void RemoveTxn(const std::string& hash);

  std::vector<std::string> GetOrder(const std::vector<std::string>& commit_txns);

  private:
  void Dfs(const std::string& hash, std::set<std::string>& res);
  void CheckGraph();

  private:
  std::map<std::string, std::set<std::string> > g_, preg_;
    std::map<std::string, int> hash2idx_;
    int idx_ = 1;
};

}  // namespace fairdag
}  // namespace resdb
