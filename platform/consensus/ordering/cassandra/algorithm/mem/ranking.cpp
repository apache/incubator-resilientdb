
#include "platform/consensus/ordering/cassandra/algorithm/mem/ranking.h"

namespace resdb {
namespace cassandra {

int Ranking::GetRank(int proposer_id) { return proposer_id; }

}  // namespace cassandra
}  // namespace resdb
