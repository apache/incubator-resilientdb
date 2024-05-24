#include "platform/consensus/ordering/fairdag_rl/algorithm/graph.h"

#include <glog/logging.h>
#include "common/utils/utils.h"


namespace resdb {
namespace fairdag_rl {

void Graph::AddTxn(const Transaction& a, const Transaction& b){
  g_[a.hash()].push_back(b.hash());
}


}  // namespace fairdag_rl
}  // namespace resdb
