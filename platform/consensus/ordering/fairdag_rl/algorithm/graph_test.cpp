/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "platform/consensus/ordering/fairdag_rl/algorithm/graph.h"

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <future>

namespace resdb {
namespace fairdag_rl {
namespace {

void AddOrder(Graph& graph, const std::vector<std::string> hash) {
  std::vector<std::unique_ptr<Transaction>> txns;

  for (int i = 0; i < hash.size(); ++i) {
    auto txn = std::make_unique<Transaction>();
    txn->set_hash(hash[i]);
    txns.push_back(std::move(txn));
  }
  for (int i = 0; i < txns.size(); ++i) {
    for (int j = i + 1; j < txns.size(); ++j) {
      graph.AddTxn(*txns[i], *txns[j]);
    }
  }
}

TEST(GraphTest, NormalCase) {
  Graph graph;

  std::vector<std::unique_ptr<Transaction>> txns;

  // commit for 1,2,3,5
  AddOrder(graph, {"hash1", "hash2", "hash3", "hash4", "hash5"});

  std::vector<std::string> commit_txns{"hash1", "hash2", "hash3", "hash5"};

  std::vector<std::string> orders = graph.GetOrder(commit_txns);

  EXPECT_EQ(orders[0], "hash1");
  EXPECT_EQ(orders[1], "hash2");
  EXPECT_EQ(orders[2], "hash3");
  EXPECT_EQ(orders[3], "hash5");

  for (int i = 0; i < txns.size(); ++i) {
    graph.RemoveTxn(txns[i].hash());
  }
}

TEST(GraphTest, SingleRing) {
  Graph graph;

  AddOrder(graph, {"hash1", "hash2", "hash3", "hash4", "hash5"});

  AddOrder(graph, {"hash1", "hash2", "hash4", "hash3", "hash5"});

  std::vector<std::string> commit_txns{"hash1", "hash2", "hash3", "hash4",
                                       "hash5"};

  std::vector<std::string> orders = graph.GetOrder(commit_txns);
  EXPECT_EQ(orders[0], "hash1");
  EXPECT_EQ(orders[1], "hash2");
  EXPECT_EQ(orders[2], "hash3");
  EXPECT_EQ(orders[3], "hash4");
  EXPECT_EQ(orders[4], "hash5");
}

TEST(GraphTest, HalfRing) {
  Graph graph;

  AddOrder(graph, {"hash1", "hash2", "hash3", "hash4", "hash5"});

  AddOrder(graph, {"hash1", "hash2", "hash4", "hash3", "hash5"});

  std::vector<std::string> commit_txns{"hash1", "hash2", "hash3", "hash5"};

  std::vector<std::string> orders = graph.GetOrder(commit_txns);
  EXPECT_EQ(orders[0], "hash1");
  EXPECT_EQ(orders[1], "hash2");
  EXPECT_EQ(orders[2], "hash3");
  EXPECT_EQ(orders[3], "hash5");
}

TEST(GraphTest, TwoRing) {
  Graph graph;

  AddOrder(graph, {"hash1", "hash2", "hash3", "hash4", "hash5", "hash6",
                   "hash7", "hash8", "hash9"});

  AddOrder(graph,
           {"hash1", "hash2", "hash4", "hash3", "hash8", "hash7", "hash6"});

  std::vector<std::string> commit_txns{"hash1", "hash2", "hash3", "hash4",
                                       "hash6", "hash7", "hash8"};

  std::vector<std::string> orders = graph.GetOrder(commit_txns);
  EXPECT_EQ(orders[0], "hash1");
  EXPECT_EQ(orders[1], "hash2");
  EXPECT_EQ(orders[2], "hash3");
  EXPECT_EQ(orders[3], "hash4");
  EXPECT_EQ(orders[4], "hash6");
  EXPECT_EQ(orders[5], "hash7");
  EXPECT_EQ(orders[6], "hash8");
}

TEST(GraphTest, Remove) {
  Graph graph;

  AddOrder(graph, {"hash1", "hash2", "hash3", "hash4", "hash5"});

  AddOrder(graph, {"hash1", "hash2", "hash4", "hash3", "hash5"});

  graph.RemoveTxn("hash3");

  std::vector<std::string> commit_txns{"hash1", "hash2", "hash3", "hash4",
                                       "hash5"};

  std::vector<std::string> orders = graph.GetOrder(commit_txns);
  EXPECT_EQ(orders[0], "hash1");
  EXPECT_EQ(orders[1], "hash2");
  EXPECT_EQ(orders[2], "hash4");
  EXPECT_EQ(orders[3], "hash5");
}

}  // namespace

}  // namespace fairdag_rl
}  // namespace resdb
