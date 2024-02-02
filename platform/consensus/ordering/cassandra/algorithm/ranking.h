#pragma once

namespace resdb {
namespace cassandra {
namespace cassandra_recv {

class Ranking {
 public:
  int GetRank(int proposer_id);
};

}  // namespace cassandra_recv
}  // namespace cassandra
}  // namespace resdb
