# Sharded 3PC on ResilientDB: Implementation Overview

## Purpose

This document gives a high-level overview of the planned architecture changes for implementing a sharded, replicated transaction system on top of ResilientDB.

The goal is to extend the existing ResilientDB-based 3PC implementation into a system with:

- multiple shards,
- multiple replicas per shard,
- one leader per shard,
- global 3PC across shard leaders,
- local PBFT replication inside each shard,
- round-robin request routing from the proxy to shard leaders.

---

## Existing Starting Point

ResilientDB currently assumes a replicated architecture where a proxy forwards client requests to a leader/primary replica. The replicas then run PBFT consensus, execute the transaction, and return responses.

Previously a 3PC implementation was added by extending the existing PBFT scaffolding. The important existing pieces are:

- `ConsensusManager3PC`: dispatches 3PC protocol messages.
- `Commitment3PC`: implements coordinator and participant logic for 3PC.
- `MessageManager::ExecuteOrderedRequest(...)`: allows a transaction to execute after a non-PBFT commit decision.
- 3PC request types in `resdb.proto`.

---

## Target Architecture

The new system will have two protocol layers:

```text
Client / Proxy
    |
    v
Shard Leader selected by round-robin routing
    |
    v
Global 3PC among shard leaders
    |
    v
Local PBFT inside each shard
    |
    v
Execution and response
```

The key idea is that 3PC and PBFT serve different roles:

- **3PC** decides whether a distributed transaction should commit across all shards.
- **PBFT** replicates the committed transaction inside each individual shard.

---

## Shard Model

The system will support a configurable shard topology.

Each shard contains:

- one shard leader,
- one or more follower replicas,
- a distinct set of data items.

For the default setup, the topology is:

```text
Shard 0: replicas 1, 2, 3, 4      leader = 1
Shard 1: replicas 5, 6, 7, 8      leader = 5
Shard 2: replicas 9, 10, 11, 12   leader = 9
Shard 3: replicas 13, 14, 15, 16  leader = 13
Proxy/client node: 17
```

Shard configuration will be read from a json file.

---

## Shard Metadata Layer

The ShardMetadata class tracks the following:

- What shard a node belongs to
- Which node is the shard leader
- Node id
- Provides access functions for convenient node/shard evaluations

Each replica will keep a ShardMetadaata object (not client).


```text
platform/consensus/ordering/sharding/shard_metadata.h
platform/consensus/ordering/sharding/shard_metadata.cpp
```

---

## Shard-Aware Communication

In the sharded topology replica communication is more complex. Requests cannot be broadcast to all replicas.

The ShardCommunicator is a wrapper for the ReplicaCommunicator that will handle inter replica communication in a shard aware manner.

1. **Send to one shard leader**  
   Used by the proxy when routing a client batch.

2. **Broadcast to all shard leaders**  
   Used by global 3PC.

3. **Broadcast to replicas in the local shard**  
   Used by local PBFT.

```text
platform/consensus/ordering/sharding/shard_communicator.h
platform/consensus/ordering/sharding/shard_communicator.cpp
```

---

## Proxy Routing Changes

The proxy will alternate which shard leader it sends transactions to.

Example with four shards:

```text
Batch 1 -> leader of shard 0
Batch 2 -> leader of shard 1
Batch 3 -> leader of shard 2
Batch 4 -> leader of shard 3
Batch 5 -> leader of shard 0
```

Yet to be implemented but will be a simple class that resembles:

class ShardRouter {
  public: 
    ShardRouter(std::shared_ptr<ShardMetadata> metadata);
    uint32_t NextShardLeader();
}

```text
platform/consensus/ordering/sharding/shard_router.h
platform/consensus/ordering/sharding/shard_router.cpp
```

---

## Global 3PC Across Shard Leaders

When a shard leader receives a transaction from the proxy, that leader becomes the global coordinator.

The global 3PC flow is:

```text
1. Coordinator sends PREPARE to all shard leaders.
2. Participating shard leaders respond VOTE_COMMIT.
3. Coordinator sends PRECOMMIT to all shard leaders.
4. Participating shard leaders respond PRECOMMIT_ACK.
5. Coordinator sends GLOBAL_COMMIT to all shard leaders.
```

Only shard leaders participate in global 3PC.

This requires changes to `Commitment3PC` so that:

- global 3PC messages are sent only to shard leaders,
- vote thresholds are based on the number of shards,
- ACK thresholds are based on the number of shards,
- non-leader replicas reject or ignore global 3PC messages.

---

## Local PBFT Inside Each Shard

After global 3PC commits, each shard leader initiates PBFT inside its own shard.

The local PBFT flow is:

```text
1. Shard leader receives GLOBAL_COMMIT.
2. Shard leader starts local PBFT for the committed transaction.
3. Local shard replicas run PBFT.
4. After local PBFT commits, replicas execute the transaction.
```

This preserves the intended layered architecture:

```text
3PC across shard leaders
PBFT within each shard
```

The main required change is that PBFT broadcasts and PBFT quorum logic must be scoped to the local shard, not to the entire deployment.

---

## Hybrid Consensus Manager

The current 3PC implementation replaced PBFT for the whole replica group. The sharded design requires both protocols to coexist.

This will be done with a hybrid consensus manager (that likely still inherits from ConsensusManagerPBFT):

```text
ConsensusManagerSharded3PC
    |
    |-- Global 3PC path
    |     uses Commitment3PC
    |
    |-- Local PBFT path
          uses existing PBFT Commitment
```

The manager dispatches messages by type:

- client requests go through the normal proxy/request path,
- global 3PC messages go to `Commitment3PC`,
- local PBFT messages go to the existing PBFT commitment logic,

---

## Request Metadata

Transactions need additional metadata so global and local protocol stages can be correlated.

Useful fields include (not final):

- global transaction ID,
- coordinator shard ID,
- global coordinator node ID,
- local shard ID,
- flag indicating global 3PC message,
- flag indicating local shard PBFT message.

These fields will be added to resdb.proto as before with 3PC specific field.

---

## Response Behavior

All shards execute the committed transaction, but only the coordinating shard should reply to the client.

The response path should use the transaction’s `coordinator_shard_id`:

```text
if local shard == coordinator shard:
    send response to client/proxy
else:
    execute locally but suppress client response
```

This may require passing shard metadata through the execution or response path.

---

## Configuration Changes

The deployment configuration must support:

- number of shards,
- replicas per shard,
- shard leader IDs,
- replica membership for each shard,
- client/proxy node IDs.

A separate `shard.config` file is utilized and has its own config generation file.

Example format:

```json
{
  "shards": [
    {
      "shard_id": 0,
      "leader_id": 1,
      "replica_ids": [1, 2, 3, 4]
    },
    {
      "shard_id": 1,
      "leader_id": 5,
      "replica_ids": [5, 6, 7, 8]
    }
  ],
  "client_ids": [17]
}
```

The ServerFactory has a new template for generating the sharded server that expects the shard.config file as input.

---

## Main Code Areas Affected

Main file additions:

```text
platform/consensus/ordering/sharding/shard_metadata.*
platform/consensus/ordering/sharding/shard_communicator.*
platform/consensus/ordering/sharding/shard_router.*
```

Expected modifications:

```text
platform/proto/resdb.proto
platform/consensus/ordering/3pc/consensus_manager_3pc.*
platform/consensus/ordering/3pc/commitment_3pc.*
platform/consensus/ordering/pbft/commitment.*
platform/consensus/ordering/pbft/response_manager.*
platform/consensus/ordering/pbft/message_manager.*
service/utils/server_factory.*
deployment config generation scripts
```

---

## Development Phases

Possible development order:

1. Add shard metadata and topology loading.
2. Add shard-aware communication helpers.
3. Modify proxy routing to round-robin among shard leaders.
4. Modify 3PC to operate only among shard leaders.
5. Add the transition from global 3PC commit to local PBFT.
6. Restrict PBFT broadcasts and quorum logic to local shard membership.
7. Add response filtering so only the coordinator shard replies.
8. Add throughput and latency measurement.

Steps 1 and 2 are mostly complete and the remaining steps are mostly modifying PBFT and 3PC logic with minimal changes.

---
