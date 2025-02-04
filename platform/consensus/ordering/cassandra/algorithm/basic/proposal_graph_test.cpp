/*
 * Copyright (c) 2019-2022 ExpoLab, UC Davis
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#include "platform/consensus/ordering/cassandra/algorithm/proposal_graph.h"

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <future>

#include "common/test/test_macros.h"

namespace resdb {
namespace cassandra {
namespace {

using ::resdb::testing::EqualsProto;
using ::testing::Test;

class ProposalGraphTest : public Test {
 public:
  ProposalGraphTest() : graph_(1) {}

 protected:
  ProposalGraph graph_;
};

TEST_F(ProposalGraphTest, AddNewProposalWithoutCommit) {
  Proposal proposal;
  proposal.mutable_header()->set_hash("1234");
  proposal.mutable_header()->set_height(1);

  EXPECT_TRUE(graph_.AddProposal(proposal));
}

TEST_F(ProposalGraphTest, AddTwoNewProposalWithoutCommit) {
  {
    Proposal proposal;
    proposal.mutable_header()->set_hash("1234");
    proposal.mutable_header()->set_height(1);

    EXPECT_TRUE(graph_.AddProposal(proposal));
  }
  {
    Proposal proposal;
    proposal.mutable_header()->set_hash("1235");
    proposal.mutable_header()->set_height(1);

    EXPECT_TRUE(graph_.AddProposal(proposal));
  }
}

TEST_F(ProposalGraphTest, AddLinkNewProposalWithoutCommit) {
  {
    Proposal proposal;
    proposal.mutable_header()->set_hash("1234");
    proposal.mutable_header()->set_height(1);

    EXPECT_TRUE(graph_.AddProposal(proposal));
  }
  graph_.IncreaseHeight();
  {
    Proposal proposal;
    proposal.mutable_header()->set_hash("1235");
    proposal.mutable_header()->set_prehash("1234");
    proposal.mutable_header()->set_height(2);

    EXPECT_TRUE(graph_.AddProposal(proposal));
  }
}

TEST_F(ProposalGraphTest, AddLinkNewProposalInvalidHeight) {
  {
    Proposal proposal;
    proposal.mutable_header()->set_hash("1234");
    proposal.mutable_header()->set_height(1);

    EXPECT_TRUE(graph_.AddProposal(proposal));
  }
  {
    Proposal proposal;
    proposal.mutable_header()->set_hash("1235");
    proposal.mutable_header()->set_prehash("1234");
    proposal.mutable_header()->set_height(1);

    EXPECT_FALSE(graph_.AddProposal(proposal));
  }
}

TEST_F(ProposalGraphTest, GetNewProposal) {
  {
    Proposal proposal;
    proposal.mutable_header()->set_hash("1234");
    proposal.mutable_header()->set_height(1);

    EXPECT_TRUE(graph_.AddProposal(proposal));
  }
  graph_.IncreaseHeight();
  {
    Proposal proposal;
    proposal.mutable_header()->set_hash("1235");
    proposal.mutable_header()->set_prehash("1234");
    proposal.mutable_header()->set_height(2);

    EXPECT_TRUE(graph_.AddProposal(proposal));
  }
  Proposal* np = graph_.GetLatestStrongestProposal();
  EXPECT_EQ(np->header().hash(), "1235");
}

TEST_F(ProposalGraphTest, GetNewProposalWithTwo) {
  {
    Proposal proposal;
    proposal.mutable_header()->set_hash("1234");
    proposal.mutable_header()->set_proposer_id(3);
    proposal.mutable_header()->set_height(1);

    EXPECT_TRUE(graph_.AddProposal(proposal));
  }
  {
    Proposal proposal;
    proposal.mutable_header()->set_hash("1235");
    proposal.mutable_header()->set_proposer_id(2);
    proposal.mutable_header()->set_height(1);
    EXPECT_TRUE(graph_.AddProposal(proposal));
  }
  graph_.IncreaseHeight();
  {
    Proposal proposal;
    proposal.mutable_header()->set_hash("1236");
    proposal.mutable_header()->set_prehash("1235");
    proposal.mutable_header()->set_proposer_id(1);
    proposal.mutable_header()->set_height(2);

    EXPECT_TRUE(graph_.AddProposal(proposal));
  }
  Proposal* np = graph_.GetLatestStrongestProposal();
  EXPECT_EQ(np->header().hash(), "1236");
}

TEST_F(ProposalGraphTest, Prepared) {
  {
    Proposal proposal;
    proposal.mutable_header()->set_hash("1231");
    proposal.mutable_header()->set_proposer_id(1);
    proposal.mutable_header()->set_height(1);

    EXPECT_TRUE(graph_.AddProposal(proposal));
  }
  {
    Proposal proposal;
    proposal.mutable_header()->set_hash("1232");
    proposal.mutable_header()->set_proposer_id(2);
    proposal.mutable_header()->set_height(1);

    EXPECT_TRUE(graph_.AddProposal(proposal));
  }
  {
    Proposal proposal;
    proposal.mutable_header()->set_hash("1233");
    proposal.mutable_header()->set_proposer_id(3);
    proposal.mutable_header()->set_height(1);

    EXPECT_TRUE(graph_.AddProposal(proposal));
  }

  Proposal* np = graph_.GetLatestStrongestProposal();
  EXPECT_EQ(np->header().hash(), "1233");
  EXPECT_EQ(np->history_size(), 1);
  graph_.IncreaseHeight();

  {
    Proposal proposal;
    proposal.mutable_header()->set_hash("2231");
    proposal.mutable_header()->set_prehash(np->header().hash());
    proposal.mutable_header()->set_proposer_id(1);
    proposal.mutable_header()->set_height(np->header().height() + 1);
    *proposal.mutable_history() = np->history();
    EXPECT_TRUE(graph_.AddProposal(proposal));
  }
  {
    Proposal proposal;
    proposal.mutable_header()->set_hash("2232");
    proposal.mutable_header()->set_prehash(np->header().hash());
    proposal.mutable_header()->set_proposer_id(2);
    proposal.mutable_header()->set_height(np->header().height() + 1);
    *proposal.mutable_history() = np->history();

    EXPECT_TRUE(graph_.AddProposal(proposal));
  }
  {
    Proposal proposal;
    proposal.mutable_header()->set_hash("2233");
    proposal.mutable_header()->set_prehash(np->header().hash());
    proposal.mutable_header()->set_proposer_id(3);
    proposal.mutable_header()->set_height(np->header().height() + 1);
    *proposal.mutable_history() = np->history();

    EXPECT_TRUE(graph_.AddProposal(proposal));
  }

  EXPECT_EQ(graph_.GetProposalState("2231"), ProposalState::New);
  EXPECT_EQ(graph_.GetProposalState("2232"), ProposalState::New);
  EXPECT_EQ(graph_.GetProposalState("2233"), ProposalState::New);

  EXPECT_EQ(graph_.GetProposalState("1231"), ProposalState::New);
  EXPECT_EQ(graph_.GetProposalState("1232"), ProposalState::New);
  EXPECT_EQ(graph_.GetProposalState("1233"), ProposalState::Prepared);
}

TEST_F(ProposalGraphTest, Commit) {
  {
    Proposal proposal;
    proposal.mutable_header()->set_hash("1231");
    proposal.mutable_header()->set_proposer_id(1);
    proposal.mutable_header()->set_height(1);

    EXPECT_TRUE(graph_.AddProposal(proposal));
  }
  {
    Proposal proposal;
    proposal.mutable_header()->set_hash("1232");
    proposal.mutable_header()->set_proposer_id(2);
    proposal.mutable_header()->set_height(1);

    EXPECT_TRUE(graph_.AddProposal(proposal));
  }
  {
    Proposal proposal;
    proposal.mutable_header()->set_hash("1233");
    proposal.mutable_header()->set_proposer_id(3);
    proposal.mutable_header()->set_height(1);

    EXPECT_TRUE(graph_.AddProposal(proposal));
  }

  Proposal* np = graph_.GetLatestStrongestProposal();
  EXPECT_EQ(np->header().hash(), "1233");
  EXPECT_EQ(np->history_size(), 1);
  graph_.IncreaseHeight();

  {
    Proposal proposal;
    proposal.mutable_header()->set_hash("2231");
    proposal.mutable_header()->set_prehash(np->header().hash());
    proposal.mutable_header()->set_proposer_id(1);
    proposal.mutable_header()->set_height(np->header().height() + 1);
    *proposal.mutable_history() = np->history();
    EXPECT_TRUE(graph_.AddProposal(proposal));
  }
  {
    Proposal proposal;
    proposal.mutable_header()->set_hash("2232");
    proposal.mutable_header()->set_prehash(np->header().hash());
    proposal.mutable_header()->set_proposer_id(2);
    proposal.mutable_header()->set_height(np->header().height() + 1);
    *proposal.mutable_history() = np->history();

    EXPECT_TRUE(graph_.AddProposal(proposal));
  }
  {
    Proposal proposal;
    proposal.mutable_header()->set_hash("2233");
    proposal.mutable_header()->set_prehash(np->header().hash());
    proposal.mutable_header()->set_proposer_id(3);
    proposal.mutable_header()->set_height(np->header().height() + 1);
    *proposal.mutable_history() = np->history();

    EXPECT_TRUE(graph_.AddProposal(proposal));
  }

  EXPECT_EQ(graph_.GetProposalState("2231"), ProposalState::New);
  EXPECT_EQ(graph_.GetProposalState("2232"), ProposalState::New);
  EXPECT_EQ(graph_.GetProposalState("2233"), ProposalState::New);

  EXPECT_EQ(graph_.GetProposalState("1231"), ProposalState::New);
  EXPECT_EQ(graph_.GetProposalState("1232"), ProposalState::New);
  EXPECT_EQ(graph_.GetProposalState("1233"), ProposalState::Prepared);

  Proposal* np2 = graph_.GetLatestStrongestProposal();
  EXPECT_EQ(np2->header().hash(), "2233");
  EXPECT_EQ(np2->history_size(), 2);
  graph_.IncreaseHeight();

  {
    Proposal proposal;
    proposal.mutable_header()->set_hash("3231");
    proposal.mutable_header()->set_prehash(np2->header().hash());
    proposal.mutable_header()->set_proposer_id(1);
    proposal.mutable_header()->set_height(np2->header().height() + 1);
    *proposal.mutable_history() = np2->history();
    EXPECT_TRUE(graph_.AddProposal(proposal));
  }
  {
    Proposal proposal;
    proposal.mutable_header()->set_hash("3232");
    proposal.mutable_header()->set_prehash(np2->header().hash());
    proposal.mutable_header()->set_proposer_id(2);
    proposal.mutable_header()->set_height(np2->header().height() + 1);
    *proposal.mutable_history() = np2->history();

    EXPECT_TRUE(graph_.AddProposal(proposal));
  }
  {
    Proposal proposal;
    proposal.mutable_header()->set_hash("3233");
    proposal.mutable_header()->set_prehash(np2->header().hash());
    proposal.mutable_header()->set_proposer_id(3);
    proposal.mutable_header()->set_height(np2->header().height() + 1);
    *proposal.mutable_history() = np2->history();

    EXPECT_TRUE(graph_.AddProposal(proposal));
  }

  EXPECT_EQ(graph_.GetProposalState("3231"), ProposalState::New);
  EXPECT_EQ(graph_.GetProposalState("3232"), ProposalState::New);
  EXPECT_EQ(graph_.GetProposalState("3233"), ProposalState::New);

  EXPECT_EQ(graph_.GetProposalState("2231"), ProposalState::New);
  EXPECT_EQ(graph_.GetProposalState("2232"), ProposalState::New);
  EXPECT_EQ(graph_.GetProposalState("2233"), ProposalState::Prepared);

  EXPECT_EQ(graph_.GetProposalState("1231"), ProposalState::New);
  EXPECT_EQ(graph_.GetProposalState("1232"), ProposalState::New);
  EXPECT_EQ(graph_.GetProposalState("1233"), ProposalState::Committed);
}

/*
TEST_F(ProposalGraphTest, GetLastCommitNone) {
 EXPECT_EQ(graph_.GetLatestCommit(), "");
}

TEST_F(ProposalGraphTest, AddNewProposalWithoutCommit) {
 Proposal proposal;
 proposal.set_hash("1234");

 EXPECT_TRUE(graph_.AddProposal(proposal));
}

TEST_F(ProposalGraphTest, ChangeState) {
  std::string hash = "1234";

  Proposal proposal;
  proposal.set_hash("1234");

  EXPECT_FALSE(graph_.ChangeState(hash, ProposalState::Committed));

   EXPECT_TRUE(graph_.AddProposal(proposal));
  EXPECT_TRUE(graph_.ChangeState(hash, ProposalState::Committed));
  EXPECT_EQ(graph_.GetLatestCommit(), "1234");
}

TEST_F(ProposalGraphTest, AddToCommit) {
  std::string hash1 = "1234";
  std::string hash2 = "2234";

  Proposal proposal;
  proposal.set_hash(hash1);
  EXPECT_TRUE(graph_.AddProposal(proposal));
  EXPECT_TRUE(graph_.ChangeState(hash1, ProposalState::Committed));
  EXPECT_EQ(graph_.GetLatestCommit(), "1234");

  proposal.set_hash(hash2);
  EXPECT_FALSE(graph_.AddProposal(proposal));
  proposal.set_prehash(hash1);
  EXPECT_TRUE(graph_.AddProposal(proposal));
}
*/

}  // namespace
}  // namespace cassandra
}  // namespace resdb
