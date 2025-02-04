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

#include "platform/consensus/ordering/cassandra/algorithm/cassandra.h"

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

class CassandraTest : public Test {
 public:
  CassandraTest() {}

 protected:
};

TEST_F(CassandraTest, NormalCase) {
  std::promise<bool> propose_done, vote_done, prepare_done, vote_prep_done,
      commit_done;
  std::future<bool> propose_done_future = propose_done.get_future(),
                    vote_done_future = vote_done.get_future();
  std::future<bool> prepare_done_future = prepare_done.get_future();
  std::future<bool> vote_prep_done_future = vote_prep_done.get_future();
  std::future<bool> commit_done_future = commit_done.get_future();

  int id = 1;
  int received_vote = 0;
  int tot = 3;
  int f = 1;
  int batch = 1;
  Cassandra cassandra_(
      id, batch, tot, f,
      [&](int type, const google::protobuf::Message& msg) {
        LOG(ERROR) << "type" << type;
        if (type == MessageType::NewProposal) {
          propose_done.set_value(true);
        } else {
          const VoteMessage* vote_msg = dynamic_cast<const VoteMessage*>(&msg);
          if (vote_msg->type() == MessageType::Vote) {
            EXPECT_EQ(vote_msg->hash(), "3234");
            vote_done.set_value(true);
          }
          if (vote_msg->type() == MessageType::Prepare) {
            EXPECT_EQ(vote_msg->hash(), "3234");
            prepare_done.set_value(true);
          }
          if (vote_msg->type() == MessageType::Voteprep) {
            EXPECT_EQ(vote_msg->hash(), "3234");
            vote_prep_done.set_value(true);
          }
        }
        return 0;
      },
      [&](const google::protobuf::Message& msg) {
        commit_done.set_value(true);
        return 0;
      });

  {
    std::unique_ptr<Transaction> txn = std::make_unique<Transaction>();
    assert(txn != nullptr);
    EXPECT_TRUE(cassandra_.ReceiveTransaction(std::move(txn)));
  }

  propose_done_future.get();
  LOG(ERROR) << "done";
  {
    Proposal proposal1;
    proposal1.set_hash("1234");
    proposal1.set_proposer_id(1);

    Proposal proposal2;
    proposal2.set_hash("2234");
    proposal2.set_proposer_id(2);

    Proposal proposal3;
    proposal3.set_hash("3234");
    proposal3.set_proposer_id(3);

    EXPECT_TRUE(cassandra_.ReceiveProposal(proposal1));
    EXPECT_TRUE(cassandra_.ReceiveProposal(proposal2));
    EXPECT_TRUE(cassandra_.ReceiveProposal(proposal3));
  }
  vote_done_future.get();
  // vote
  {
    VoteMessage message;
    message.set_type(MessageType::Vote);
    message.set_hash("3234");
    message.set_proposer_id(1);
    EXPECT_TRUE(cassandra_.ReceiveVote(message));
    message.set_proposer_id(2);
    EXPECT_TRUE(cassandra_.ReceiveVote(message));
    message.set_proposer_id(3);
    EXPECT_TRUE(cassandra_.ReceiveVote(message));
    // TODO test same voter
  }
  prepare_done_future.get();
  {
    VoteMessage message;
    message.set_type(MessageType::Prepare);
    message.set_hash("3234");
    message.set_proposer_id(1);
    EXPECT_TRUE(cassandra_.ReceiveVote(message));
    message.set_proposer_id(2);
    EXPECT_TRUE(cassandra_.ReceiveVote(message));
    message.set_proposer_id(3);
    EXPECT_TRUE(cassandra_.ReceiveVote(message));
    // TODO test same voter
  }
  vote_prep_done_future.get();
  {
    VoteMessage message;
    message.set_type(MessageType::Voteprep);
    message.set_hash("3234");
    message.set_proposer_id(1);
    EXPECT_TRUE(cassandra_.ReceiveVote(message));
    message.set_proposer_id(2);
    EXPECT_TRUE(cassandra_.ReceiveVote(message));
    message.set_proposer_id(3);
    EXPECT_TRUE(cassandra_.ReceiveVote(message));
    // TODO test same voter
  }
  commit_done_future.get();
}

TEST_F(CassandraTest, ProposalTimeout) {
  std::promise<bool> propose_done, vote_done, prepare_done, vote_prep_done,
      commit_done;
  std::future<bool> propose_done_future = propose_done.get_future(),
                    vote_done_future = vote_done.get_future();
  std::future<bool> prepare_done_future = prepare_done.get_future();
  std::future<bool> vote_prep_done_future = vote_prep_done.get_future();
  std::future<bool> commit_done_future = commit_done.get_future();

  int id = 1;
  int received_vote = 0;
  int tot = 3;
  int f = 1;
  int batch = 1;
  Cassandra cassandra_(
      id, batch, tot, f,
      [&](int type, const google::protobuf::Message& msg) {
        LOG(ERROR) << "type" << type;
        if (type == MessageType::NewProposal) {
          propose_done.set_value(true);
        } else {
          const VoteMessage* vote_msg = dynamic_cast<const VoteMessage*>(&msg);
          if (vote_msg->type() == MessageType::Vote) {
            LOG(ERROR) << "??? hash:" << vote_msg->hash();
            EXPECT_EQ(vote_msg->hash(), "3234");
            vote_done.set_value(true);
            LOG(ERROR) << "done";
          }
          if (vote_msg->type() == MessageType::Prepare) {
            EXPECT_EQ(vote_msg->hash(), "3234");
            prepare_done.set_value(true);
          }
          if (vote_msg->type() == MessageType::Voteprep) {
            EXPECT_EQ(vote_msg->hash(), "3234");
            vote_prep_done.set_value(true);
          }
        }
        return 0;
      },
      [&](const google::protobuf::Message& msg) {
        commit_done.set_value(true);
        return 0;
      });

  {
    std::unique_ptr<Transaction> txn = std::make_unique<Transaction>();
    assert(txn != nullptr);
    EXPECT_TRUE(cassandra_.ReceiveTransaction(std::move(txn)));
  }

  propose_done_future.get();
  LOG(ERROR) << "done";
  {
    Proposal proposal1;
    proposal1.set_hash("3234");
    proposal1.set_proposer_id(1);

    /*
       Proposal proposal2;
       proposal2.set_hash("2234");
       proposal2.set_proposer_id(2);

       Proposal proposal3;
       proposal3.set_hash("3234");
       proposal3.set_proposer_id(3);
       */

    EXPECT_TRUE(cassandra_.ReceiveProposal(proposal1));
    // EXPECT_TRUE(cassandra_.ReceiveProposal(proposal2));
    // EXPECT_TRUE(cassandra_.ReceiveProposal(proposal3));
  }
  vote_done_future.get();
  // vote
  {
    VoteMessage message;
    message.set_type(MessageType::Vote);
    message.set_hash("3234");
    message.set_proposer_id(1);
    EXPECT_TRUE(cassandra_.ReceiveVote(message));
    message.set_proposer_id(2);
    EXPECT_TRUE(cassandra_.ReceiveVote(message));
    message.set_proposer_id(3);
    EXPECT_TRUE(cassandra_.ReceiveVote(message));
    // TODO test same voter
  }
  prepare_done_future.get();
  {
    VoteMessage message;
    message.set_type(MessageType::Prepare);
    message.set_hash("3234");
    message.set_proposer_id(1);
    EXPECT_TRUE(cassandra_.ReceiveVote(message));
    message.set_proposer_id(2);
    EXPECT_TRUE(cassandra_.ReceiveVote(message));
    message.set_proposer_id(3);
    EXPECT_TRUE(cassandra_.ReceiveVote(message));
    // TODO test same voter
  }
  vote_prep_done_future.get();
  {
    VoteMessage message;
    message.set_type(MessageType::Voteprep);
    message.set_hash("3234");
    message.set_proposer_id(1);
    EXPECT_TRUE(cassandra_.ReceiveVote(message));
    message.set_proposer_id(2);
    EXPECT_TRUE(cassandra_.ReceiveVote(message));
    message.set_proposer_id(3);
    EXPECT_TRUE(cassandra_.ReceiveVote(message));
    // TODO test same voter
  }
  commit_done_future.get();
}

TEST_F(CassandraTest, VoteTimeout) {
  std::promise<bool> propose_done, vote_done, prepare_done, vote_prep_done,
      commit_done, propose2_done, vote2_done;
  std::future<bool> propose_done_future = propose_done.get_future(),
                    vote_done_future = vote_done.get_future();
  std::future<bool> prepare_done_future = prepare_done.get_future();
  std::future<bool> vote_prep_done_future = vote_prep_done.get_future();
  std::future<bool> commit_done_future = commit_done.get_future();
  std::future<bool> propose2_done_future = propose2_done.get_future();
  std::future<bool> vote2_done_future = vote2_done.get_future();

  int id = 1;
  int received_vote = 0;
  int tot = 3;
  int f = 1;
  int batch = 1;
  int num = 0;
  Cassandra cassandra_(
      id, batch, tot, f,
      [&](int type, const google::protobuf::Message& msg) {
        LOG(ERROR) << "type" << type;
        if (type == MessageType::NewProposal) {
          if (num++ == 0) {
            propose_done.set_value(true);
          } else {
            propose2_done.set_value(true);
          }

        } else {
          const VoteMessage* vote_msg = dynamic_cast<const VoteMessage*>(&msg);
          if (vote_msg->type() == MessageType::Vote) {
            LOG(ERROR) << "??? hash:" << vote_msg->hash();
            EXPECT_EQ(vote_msg->hash(), "3234");
            if (received_vote++ == 0) {
              vote_done.set_value(true);
            } else {
              vote2_done.set_value(true);
            }
            LOG(ERROR) << "done";
          }
          if (vote_msg->type() == MessageType::Prepare) {
            EXPECT_EQ(vote_msg->hash(), "3234");
            prepare_done.set_value(true);
          }
          if (vote_msg->type() == MessageType::Voteprep) {
            EXPECT_EQ(vote_msg->hash(), "3234");
            vote_prep_done.set_value(true);
          }
        }
        return 0;
      },
      [&](const google::protobuf::Message& msg) {
        commit_done.set_value(true);
        return 0;
      });

  {
    std::unique_ptr<Transaction> txn = std::make_unique<Transaction>();
    assert(txn != nullptr);
    EXPECT_TRUE(cassandra_.ReceiveTransaction(std::move(txn)));
  }

  propose_done_future.get();
  LOG(ERROR) << "done";
  {
    Proposal proposal1;
    proposal1.set_hash("3234");
    proposal1.set_proposer_id(1);

    Proposal proposal2;
    proposal2.set_hash("2234");
    proposal2.set_proposer_id(2);

    Proposal proposal3;
    proposal3.set_hash("3234");
    proposal3.set_proposer_id(3);

    EXPECT_TRUE(cassandra_.ReceiveProposal(proposal1));
    EXPECT_TRUE(cassandra_.ReceiveProposal(proposal2));
    EXPECT_TRUE(cassandra_.ReceiveProposal(proposal3));
  }
  vote_done_future.get();
  // vote
  {
    VoteMessage message;
    message.set_type(MessageType::Vote);
    message.set_hash("3234");
    message.set_proposer_id(1);
    EXPECT_TRUE(cassandra_.ReceiveVote(message));
    /*
    message.set_proposer_id(2);
    EXPECT_TRUE(cassandra_.ReceiveVote(message));
    message.set_proposer_id(3);
    EXPECT_TRUE(cassandra_.ReceiveVote(message));
    */
    // TODO test same voter
  }
  propose2_done_future.get();
  {
    {
      Proposal proposal1;
      proposal1.set_hash("3234");
      proposal1.set_proposer_id(1);

      Proposal proposal2;
      proposal2.set_hash("2234");
      proposal2.set_proposer_id(2);

      Proposal proposal3;
      proposal3.set_hash("3234");
      proposal3.set_proposer_id(3);

      EXPECT_TRUE(cassandra_.ReceiveProposal(proposal1));
      EXPECT_TRUE(cassandra_.ReceiveProposal(proposal2));
      EXPECT_TRUE(cassandra_.ReceiveProposal(proposal3));
    }
    vote2_done_future.get();
    // vote
    {
      VoteMessage message;
      message.set_type(MessageType::Vote);
      message.set_hash("3234");
      message.set_proposer_id(1);
      EXPECT_TRUE(cassandra_.ReceiveVote(message));
      message.set_proposer_id(2);
      EXPECT_TRUE(cassandra_.ReceiveVote(message));
      message.set_proposer_id(3);
      EXPECT_TRUE(cassandra_.ReceiveVote(message));
      // TODO test same voter
    }
  }
  prepare_done_future.get();
  {
    VoteMessage message;
    message.set_type(MessageType::Prepare);
    message.set_hash("3234");
    message.set_proposer_id(1);
    EXPECT_TRUE(cassandra_.ReceiveVote(message));
    message.set_proposer_id(2);
    EXPECT_TRUE(cassandra_.ReceiveVote(message));
    message.set_proposer_id(3);
    EXPECT_TRUE(cassandra_.ReceiveVote(message));
    // TODO test same voter
  }
  vote_prep_done_future.get();
  {
    VoteMessage message;
    message.set_type(MessageType::Voteprep);
    message.set_hash("3234");
    message.set_proposer_id(1);
    EXPECT_TRUE(cassandra_.ReceiveVote(message));
    message.set_proposer_id(2);
    EXPECT_TRUE(cassandra_.ReceiveVote(message));
    message.set_proposer_id(3);
    EXPECT_TRUE(cassandra_.ReceiveVote(message));
    // TODO test same voter
  }
  commit_done_future.get();
}

}  // namespace
}  // namespace cassandra
}  // namespace resdb
