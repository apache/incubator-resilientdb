
#include "platform/consensus/ordering/cassandra/algorithm/mem_recovery/ranking.h"

namespace resdb {
namespace cassandra {
namespace cassandra_recv {

int Ranking::GetRank(int proposer_id) { return proposer_id; }

}  // namespace cassandra_recv
}  // namespace cassandra
}  // namespace resdb
