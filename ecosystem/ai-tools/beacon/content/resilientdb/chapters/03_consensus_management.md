<!--
  ~ Licensed to the Apache Software Foundation (ASF) under one
  ~ or more contributor license agreements.  See the NOTICE file
  ~ distributed with this work for additional information
  ~ regarding copyright ownership.  The ASF licenses this file
  ~ to you under the Apache License, Version 2.0 (the
  ~ "License"); you may not use this file except in compliance
  ~ with the License.  You may obtain a copy of the License at
  ~
  ~   http://www.apache.org/licenses/LICENSE-2.0
  ~
  ~ Unless required by applicable law or agreed to in writing,
  ~ software distributed under the License is distributed on an
  ~ "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
  ~ KIND, either express or implied.  See the License for the
  ~ specific language governing permissions and limitations
  ~ under the License.
-->

---
layout: default
title: 'Chapter 3: Consensus Management'
parent: 'ResilientDB'
nav_order: 3
---

# Chapter 3: Consensus Management

In the previous chapter, [Chapter 2: Network Communication](02_network_communication.md), we learned how the different computers (replicas) in the ResilientDB network reliably send and receive messages using components like `ReplicaCommunicator` and `ServiceNetwork`. It's like having a dependable postal service and phone system.

But _what_ important conversations are happening over these communication lines? How do the replicas use these messages to agree on the single, correct order of operations, even if some of them are slow, faulty, or even trying to cheat?

Welcome to Chapter 3! We'll dive into the brain of the agreement process: **Consensus Management**, primarily handled by the `ConsensusManager`.

## The Need for Agreement: Why Consensus?

Imagine a shared digital bulletin board where several people can post notes (transactions). Everyone needs to see the _same notes_ in the _same order_.

- What if two people try to post a note at the exact same time? Which one comes first?
- What if one person's internet connection is flaky, and their notes arrive late or out of order for some viewers?
- Worse, what if someone tries to sneakily change the order of notes already posted or tries to prevent valid notes from appearing?

A simple database running on one computer doesn't have these problems. But ResilientDB is a _distributed_ database, run by a team of replicas. To maintain a consistent and trustworthy database state across all replicas, they _must_ agree on the exact sequence of transactions. This process of reaching agreement in a distributed system, especially one where some participants might be unreliable or malicious (these are called "Byzantine faults"), is called **consensus**.

ResilientDB uses sophisticated algorithms like **Practical Byzantine Fault Tolerance (PBFT)** to achieve this. PBFT is designed to work correctly even if a certain number of replicas (up to `f` out of `3f+1` total replicas) are behaving incorrectly.

## Meet the Chief Negotiator: `ConsensusManager`

The `ConsensusManager` is the core component within each ResilientDB replica responsible for implementing and orchestrating the consensus algorithm (like PBFT).

**Analogy:** Think of the `ConsensusManager` as the **chief negotiator** or **meeting facilitator** in a high-stakes meeting (ordering transactions). Its job is to:

1.  Receive proposed transactions (often initiated by clients via [Chapter 1](01_client_interaction__kvclient___utxoclient___contractclient___transactionconstructor_.md)).
2.  Guide the discussion (exchange messages with other replicas' `ConsensusManager`s using the tools from [Chapter 2](02_network_communication__replicacommunicator___servicenetwork_.md)).
3.  Ensure that all honest participants reach the _same decision_ about the transaction order, despite potential disagreements or disruptions from faulty participants.
4.  Announce the final, agreed-upon order so the transaction can be executed.

It's the engine that drives the replicas towards a unified view of the transaction history.

## How Consensus Works (A Simplified PBFT View)

PBFT works in phases, involving message exchanges between replicas. Here's a simplified overview:

1.  **Client Request:** A client sends a transaction request (e.g., "Set key 'A' to value '10'") to the ResilientDB network, usually targeting the current leader replica (called the "Primary"). (See [Chapter 1](01_client_interaction__kvclient___utxoclient___contractclient___transactionconstructor_.md)).
2.  **Pre-Prepare (Proposal):** The Primary replica receives the client request. If it deems it valid, it assigns a sequence number (determining its order) and broadcasts a `PRE-PREPARE` message to all other replicas (called "Backups"). This message basically says, "I propose we process this transaction with sequence number N."
3.  **Prepare (Voting):** When a Backup replica receives the `PRE-PREPARE` message, it validates it. If it looks good (correct sequence number, valid request format, etc.), the Backup broadcasts a `PREPARE` message to all other replicas (including the Primary). This message means, "I've seen the proposal for sequence N, and it looks okay to me. I'm prepared to accept it."
4.  **Commit (Confirming):** Each replica (Primary and Backups) waits until it receives enough (`2f+1` including its own vote, where `f` is the max number of faulty replicas tolerated) matching `PREPARE` messages for the same sequence number N from different replicas. Seeing these votes convinces the replica that enough _other_ honest replicas also agree on the proposal. The replica then broadcasts a `COMMIT` message, signifying "I've seen enough agreement ('Prepared' certificate); I am now ready to commit to sequence number N."
5.  **Execute (Decision):** Each replica waits until it receives enough (`2f+1`) matching `COMMIT` messages for sequence number N. This forms the final proof ("Committed" certificate) that the transaction is agreed upon by the network. The replica now knows the transaction is finalized in the agreed order. It can safely execute the transaction (e.g., actually update the key 'A' to '10') and potentially store the result. This execution step is handled by components discussed in [Chapter 5: Transaction Execution (TransactionManager / TransactionExecutor)](05_transaction_execution__transactionmanager___transactionexecutor_.md).

This multi-phase process ensures that even if the Primary is malicious or some Backups are faulty, the honest replicas will eventually agree on the same order for valid transactions.

## Code Sneak Peek: The `ConsensusManagerPBFT`

ResilientDB implements PBFT consensus within the `ConsensusManagerPBFT` class. It inherits from a base `ConsensusManager` and handles the specifics of PBFT.

One of the key functions is `ConsensusCommit`. This function acts like the central dispatcher within the `ConsensusManagerPBFT`. When a message related to consensus arrives from the network (via the `ServiceNetwork` from [Chapter 2](02_network_communication__replicacommunicator___servicenetwork_.md)), it's eventually passed to this function.

```cpp
// Simplified from platform/consensus/ordering/pbft/consensus_manager_pbft.cpp

// This function receives consensus-related messages after initial network processing.
int ConsensusManagerPBFT::ConsensusCommit(std::unique_ptr<Context> context,
                                          std::unique_ptr<Request> request) {
  // ... (Code for handling view changes, checking if ready) ...

  // Call the internal function to handle different message types
  int ret = InternalConsensusCommit(std::move(context), std::move(request));

  // ... (Code for handling outcomes, potential view changes) ...
  return ret;
}

// This function routes the message based on its type.
int ConsensusManagerPBFT::InternalConsensusCommit(
    std::unique_ptr<Context> context, std::unique_ptr<Request> request) {

  // Check the type of message received
  switch (request->type()) {
    // If it's a brand new transaction proposal from a client (relayed by primary):
    case Request::TYPE_NEW_TXNS:
      // Pass it to the 'Commitment' component to start the PBFT process
      // (This happens on the Primary replica)
      return commitment_->ProcessNewRequest(std::move(context), std::move(request));

    // If it's a PRE-PREPARE message (proposal from the primary):
    case Request::TYPE_PRE_PREPARE:
      // Pass it to the 'Commitment' component to handle the proposal
      // (This happens on Backup replicas)
      return commitment_->ProcessProposeMsg(std::move(context), std::move(request));

    // If it's a PREPARE message (a vote):
    case Request::TYPE_PREPARE:
      // Pass it to 'Commitment' to collect Prepare votes
      return commitment_->ProcessPrepareMsg(std::move(context), std::move(request));

    // If it's a COMMIT message (a confirmation vote):
    case Request::TYPE_COMMIT:
      // Pass it to 'Commitment' to collect Commit votes
      return commitment_->ProcessCommitMsg(std::move(context), std::move(request));

    // Handle other message types like Checkpoints, View Changes, Queries...
    case Request::TYPE_CHECKPOINT:
      return checkpoint_manager_->ProcessCheckPoint(std::move(context), std::move(request));
    case Request::TYPE_VIEWCHANGE:
      return view_change_manager_->ProcessViewChange(std::move(context), std::move(request));
    // ... other cases ...
  }
  return 0; // Default return
}
```

**Explanation:**

- `ConsensusCommit` is the entry point. It might do some initial checks (like handling view changes, which occur if the leader is suspected to be faulty).
- `InternalConsensusCommit` does the main work: it looks at the `request->type()` to see what kind of message it is (`PRE_PREPARE`, `PREPARE`, `COMMIT`, etc.).
- Based on the type, it calls the appropriate processing function, often belonging to helper components like `commitment_`, `checkpoint_manager_`, or `view_change_manager_`.
- The `commitment_` object (an instance of the `Commitment` class) is particularly important, as it manages the core state machine of the PBFT protocol (tracking the pre-prepare, prepare, and commit phases for each transaction).

This shows how `ConsensusManagerPBFT` acts as a coordinator, receiving messages and delegating the detailed processing to specialized sub-components.

## Internal Implementation Walkthrough

Let's visualize the simplified PBFT flow for a client request:

```mermaid
sequenceDiagram
    participant Client
    participant Primary as "Primary Replica (ConsensusManager)"
    participant Backup1 as "Backup Replica 1 (CM)"
    participant Backup2 as "Backup Replica 2 (CM)"
    participant Executor as "Transaction Executor"

    Client->>Primary: Send Transaction Request (e.g., SET A=10)
    Primary->>Primary: Assign Sequence Number (e.g., N=5)
    Primary->>Backup1: Broadcast PRE-PREPARE (N=5, Req)
    Primary->>Backup2: Broadcast PRE-PREPARE (N=5, Req)
    Backup1->>Primary: Send PREPARE (N=5)
    Backup1->>Backup2: Send PREPARE (N=5)
    Backup2->>Primary: Send PREPARE (N=5)
    Backup2->>Backup1: Send PREPARE (N=5)
    Note over Primary, Backup1, Backup2: Each replica collects PREPARE messages. If enough (2f+1) match, proceed.
    Primary->>Backup1: Send COMMIT (N=5)
    Primary->>Backup2: Send COMMIT (N=5)
    Backup1->>Primary: Send COMMIT (N=5)
    Backup1->>Backup2: Send COMMIT (N=5)
    Backup2->>Primary: Send COMMIT (N=5)
    Backup2->>Backup1: Send COMMIT (N=5)
    Note over Primary, Backup1, Backup2: Each replica collects COMMIT messages. If enough (2f+1) match, consensus reached!
    Primary->>Executor: Execute Transaction (N=5)
    Backup1->>Executor: Execute Transaction (N=5)
    Backup2->>Executor: Execute Transaction (N=5)
    Note over Executor: Transaction Execution discussed in Chapter 5.
    Executor->>Primary: Execution Result
    Primary->>Client: Send Response (Success)
    Backup1->>Executor: Execution Result
    Backup2->>Executor: Execution Result
```

**Diving Deeper into Code:**

The `ConsensusManagerPBFT` is set up when a replica starts. Its constructor initializes various helper components:

```cpp
// Simplified from platform/consensus/ordering/pbft/consensus_manager_pbft.cpp

ConsensusManagerPBFT::ConsensusManagerPBFT(
    const ResDBConfig& config, std::unique_ptr<TransactionManager> executor,
    /* ... other params ... */)
    : ConsensusManager(config), // Initialize base class
      system_info_(std::make_unique<SystemInfo>(config)), // Holds view/primary info
      checkpoint_manager_( /* ... */ ), // Manages state saving ([Chapter 7](07_checkpointing___recovery__checkpointmanager___recovery_.md))
      message_manager_( /* ... */ ), // Helps manage message state ([Chapter 4](04_message_transaction_collection__transactioncollector___messagemanager_.md))
      commitment_(std::make_unique<Commitment>(config_, message_manager_.get(), /* ... */)), // Handles PBFT phases
      query_( /* ... */ ), // Handles read requests
      response_manager_( /* ... */ ), // Manages client responses
      performance_manager_( /* ... */ ), // For performance tracking
      view_change_manager_(std::make_unique<ViewChangeManager>(config_, /* ... */)), // Handles leader changes
      recovery_( /* ... */ ) // Handles startup recovery ([Chapter 7](07_checkpointing___recovery__checkpointmanager___recovery_.md))
{
    // ... Further initialization ...
    LOG(INFO) << "ConsensusManagerPBFT initialized.";
}
```

**Explanation:**

- The constructor creates instances of many important classes.
- `Commitment`: As mentioned, this is central to processing `PRE-PREPARE`, `PREPARE`, and `COMMIT` messages.
- `ViewChangeManager`: Watches for signs that the Primary might be faulty and manages the process of electing a new one if needed.
- `CheckPointManager`: Periodically helps replicas agree on a stable, saved state so they don't have to keep all messages forever ([Chapter 7](07_checkpointing___recovery__checkpointmanager___recovery_.md)).
- `MessageManager`: Often works closely with `Commitment` to store and retrieve messages associated with different sequence numbers ([Chapter 4](04_message_transaction_collection__transactioncollector___messagemanager_.md)).
- `TransactionManager` (passed in as `executor`): This is the link to actually executing the transaction once consensus is reached ([Chapter 5](05_transaction_execution__transactionmanager___transactionexecutor_.md)).

The `ConsensusManagerPBFT` acts as the high-level coordinator, relying on these specialized components to handle the details of each part of the consensus process.

## Conclusion

You've now learned about the critical role of the **Consensus Manager (`ConsensusManagerPBFT`)** in ResilientDB. It's the engine that ensures all replicas agree on the order of transactions, providing the foundation for the database's reliability and fault tolerance.

- We saw why **consensus** is essential in a distributed system like ResilientDB.
- We met the `ConsensusManager` as the **chief negotiator**, orchestrating agreement using algorithms like PBFT.
- We walked through a simplified **PBFT flow** (Pre-Prepare, Prepare, Commit).
- We looked at the `ConsensusCommit` function in `ConsensusManagerPBFT` and how it delegates tasks based on message types to components like `Commitment` and `ViewChangeManager`.

The `ConsensusManager` deals with deciding the _order_. But before consensus can even begin, the replicas need an efficient way to gather all the incoming client transaction requests and the flood of consensus messages (Prepare, Commit votes) being exchanged. How are these messages collected and organized?

That's what we'll explore in the next chapter!

**Next:** [Chapter 4: Message/Transaction Collection](04_message_transaction_collection.md)

---

Generated by [AI Codebase Knowledge Builder](https://github.com/The-Pocket/Tutorial-Codebase-Knowledge)
