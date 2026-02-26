#pragma once

//#define GOPOA
//#define GOTPOA
//#define GOTPOR

//#define NOPOA

namespace resdb {
namespace autobahn {

/*
enum ProposalState {
  None = 0,
  New = 1,
  Voted = 2,
  Prepared = 3,
  PreCommit = 4,
  Committed = 5,
};
*/

enum ProposalState {
  New = 0,
  PoA = 1,
  PoR = 2,
  Committed = 3
};

}
}  // namespace resdb
