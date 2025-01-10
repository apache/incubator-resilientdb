#pragma once

namespace resdb {
namespace cassandra_cft {

class Ranking {
 public:
  int GetRank(int proposer_id);
};

}  // namespace cassandra_cft
}  // namespace resdb
