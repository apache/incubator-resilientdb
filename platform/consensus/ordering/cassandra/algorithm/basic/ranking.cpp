
#include "platform/consensus/ordering/cassandra/algorithm/basic/ranking.h"

namespace resdb {
namespace cassandra {
namespace basic {

int Ranking::GetRank(int proposer_id) { return proposer_id; }

}  // namespace basic
}  // namespace cassandra
}  // namespace resdb
