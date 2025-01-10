
#include "platform/consensus/ordering/cassandra_cft/algorithm/ranking.h"

namespace resdb {
namespace cassandra_cft {

int Ranking::GetRank(int proposer_id) { return proposer_id; }

}  // namespace cassandra_cft
}  // namespace resdb
