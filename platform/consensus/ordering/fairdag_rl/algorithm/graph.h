#pragma once

#include <map>
#include "platform/consensus/ordering/fairdag_rl/proto/proposal.pb.h"

namespace resdb {
namespace fairdag_rl {

class Graph {
public:
  void AddTxn(const Transaction& a, const Transaction& b);

  private:
  std::map<std::string, std::vector<std::string> > g_;
};

}  // namespace fairdag
}  // namespace resdb
