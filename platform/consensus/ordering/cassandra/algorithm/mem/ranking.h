#pragma once

namespace resdb {
namespace cassandra {

class Ranking {
 public:
  int GetRank(int proposer_id);
};

}  // namespace cassandra
}  // namespace resdb
