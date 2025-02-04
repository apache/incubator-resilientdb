
#include "platform/consensus/ordering/cassandra/algorithm/fast_path/ranking.h"

namespace resdb {
namespace cassandra {
namespace cassandra_fp {

int Ranking::GetRank(int proposer_id) { return proposer_id; }

}  // namespace cassandra_fp
}  // namespace cassandra
}  // namespace resdb
