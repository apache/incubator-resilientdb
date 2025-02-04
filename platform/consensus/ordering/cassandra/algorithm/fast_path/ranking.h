#pragma once

namespace resdb {
namespace cassandra {
namespace cassandra_fp {

class Ranking {
 public:
  int GetRank(int proposer_id);
};

}  // namespace cassandra_fp
}  // namespace cassandra
}  // namespace resdb
