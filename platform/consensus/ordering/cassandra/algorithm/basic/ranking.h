#pragma once

namespace resdb {
namespace cassandra {
namespace basic {

class Ranking {
 public:
  int GetRank(int proposer_id);
};

}  // namespace basic
}  // namespace cassandra
}  // namespace resdb
