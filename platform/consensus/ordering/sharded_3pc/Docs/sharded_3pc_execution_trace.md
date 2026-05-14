# Sharded 3PC Execution Trace

This note gives a lightweight map of the sharded 3PC/PBFT implementation. It is
intended as a file-oriented trace of the execution path: how the new sharded
components are launched, how requests move through global 3PC, how they enter
local PBFT, and how responses are limited to the coordinator shard.

## High-Level Architecture

The sharded implementation layers two consensus protocols:

```text
client / proxy
  -> round-robin shard leader selection
  -> global 3PC across shard leaders
  -> local PBFT inside every shard
  -> execution on every shard
  -> client responses only from the coordinator shard
```

The flat 3PC implementation replaced PBFT as the ordering protocol for the
whole replica group. The sharded implementation keeps both protocols alive:

- `Commitment3PC` provides the global commit protocol.
- PBFT `Commitment` provides local replication inside a shard.
- `ConsensusManagerSharded3PC` owns and dispatches between both paths.

## Startup And Configuration

The sharded entrypoint is `service/kv/kv_service_sharded_3pc.cpp`. It is based
on the existing KV service entrypoints, but it accepts both the normal ResDB
server config and a shard config:

```text
kv_service_sharded_3pc <server_config> <shard_config> <private_key> <cert_file>
```

The service calls:

```cpp
CustomGenerateResDBServerWithShardConfig<ConsensusManagerSharded3PC>(...)
```

That helper is defined in `service/utils/server_factory.h`. It constructs the
normal network-wide `ResDBConfig`, then constructs the consensus manager with:

```cpp
ConsensusManagerSharded3PC(config, shard_config_path, executor)
```

The launch scripts are:

- `service/tools/kv/server_tools/generate_config_sharded.sh`
  - generates the normal `server.config` / `service.config`
  - generates `service/tools/config/shard/shard.config`
  - assigns contiguous server node IDs to shards
  - assigns the first node in each shard as the shard leader
- `service/tools/kv/server_tools/start_kv_service_sharded_3pc.sh`
  - launches all server replica nodes with `kv_service_sharded_3pc`
  - launches client/proxy nodes with the same binary
  - writes logs as `server0.log`, `server1.log`, ..., and `client.log`

## Shard Topology

Shard topology is parsed by:

```text
platform/consensus/ordering/sharded_3pc/shard_metadata.{h,cpp}
```

`ShardMetadata` reads `shard.config` and builds lookup tables for:

- shard ID -> shard record
- node ID -> shard ID
- shard ID -> leader node ID
- all shard leaders
- client/proxy node IDs

Server replicas must belong to exactly one shard. Client/proxy nodes may appear
in `client_ids` and do not have a local shard. That distinction is important:
server nodes install global 3PC and local PBFT commitments, while client nodes
install only the sharded response manager.

## Sharded Consensus Manager

The central dispatcher is:

```text
platform/consensus/ordering/sharded_3pc/consensus_manager_sharded_3pc.{h,cpp}
```

`ConsensusManagerSharded3PC` inherits from `ConsensusManagerPBFT`, but it
defers the base PBFT commitment, response manager, and recovery setup so it can
install sharded versions.

For a server replica, it creates:

- `ShardMetadata`
- a shard-local `ResDBConfig` for PBFT quorum sizing
- `ShardCommunicator`
- `ShardPBFTCommitment` as the inherited `commitment_`
- `Shard3PCCommitment` as `commitment_3pc_`

For a client/proxy node, it creates:

- `ShardMetadata`
- `ShardedResponseManager`

The dispatcher sends request types to the proper layer:

- `TYPE_CLIENT_REQUEST` and `TYPE_RESPONSE` go to the response manager.
- `TYPE_NEW_TXNS` enters global 3PC at the selected shard leader.
- `TYPE_3PC_*` messages go to `Shard3PCCommitment`.
- `TYPE_SHARD_PBFT_NEW_TXN` marks the handoff from global 3PC to local PBFT.
- `TYPE_PRE_PREPARE`, `TYPE_PREPARE`, and `TYPE_COMMIT` go to local PBFT.

The trace logs added for debugging are emitted from this manager:

- `[sharded_3pc_trace]` for global 3PC messages
- `[shard_pbft_trace]` for local PBFT messages
- `[sharded_handoff]` when global 3PC starts local PBFT

## Proxy Routing

The proxy path is implemented by:

```text
platform/consensus/ordering/sharded_3pc/shard_router.{h,cpp}
platform/consensus/ordering/sharded_3pc/sharded_response_manager.{h,cpp}
```

`ShardRouter` rotates deterministically through shard IDs and returns the next
`{shard_id, leader_id}` pair.

`ShardedResponseManager` extends the existing PBFT `ResponseManager`. When a
client request is batched, it chooses the next shard leader and annotates the
outgoing `TYPE_NEW_TXNS` request with sharded metadata:

- `global_txn_id`
- `coordinator_shard_id`
- `global_coordinator_id`
- `is_global_3pc`

It also records `local_id -> shard route` so that response counting and timeout
resends use the same coordinator shard instead of switching shards mid-request.
Response quorum is based on the coordinator shard size, not the full deployment.

## Shard-Aware Communication

Shard-targeted sends are handled by:

```text
platform/consensus/ordering/sharded_3pc/shard_communicator.{h,cpp}
```

`ShardCommunicator` wraps the existing `ReplicaCommunicator`. It does not
replace networking; it only selects targets before calling the existing send
path.

The important target patterns are:

- send to one node
- broadcast to one shard
- broadcast to the local shard
- broadcast to all shard leaders
- send to a shard leader

This is how the implementation avoids using ResilientDB's flat broadcast list
for sharded consensus messages.

## Global 3PC Layer

The sharded 3PC wrapper is:

```text
platform/consensus/ordering/sharded_3pc/shard_3pc_commitment.{h,cpp}
```

`Shard3PCCommitment` inherits from the flat `Commitment3PC`. The flat 3PC code
still owns the phase logic: prepare, vote, precommit, ack, global commit, and
global abort.

The sharded wrapper changes the parts that depend on topology:

- global 3PC broadcasts go to shard leaders only
- directed 3PC messages are sent through `ShardCommunicator`
- expected 3PC participants equals `ShardMetadata::NumShards()`
- global commit does not execute directly

Instead of executing directly on global commit, `Shard3PCCommitment` calls the
`global_commit_handler_` callback installed by `ConsensusManagerSharded3PC`.
That callback enters:

```cpp
ConsensusManagerSharded3PC::StartLocalPBFTFromGlobalCommit(...)
```

## Local PBFT Layer

The local PBFT wrapper is:

```text
platform/consensus/ordering/sharded_3pc/shard_pbft_commitment.{h,cpp}
```

`ShardPBFTCommitment` inherits from PBFT `Commitment`. The existing PBFT logic
still handles pre-prepare, prepare, commit, execution, checkpointing, and
response sending.

The wrapper changes the topology-sensitive parts:

- PBFT consensus broadcasts go only to local shard replicas
- directed PBFT consensus sends use `ShardCommunicator`
- globally committed requests are converted into local `TYPE_PRE_PREPARE`
  messages by `ProcessGlobalCommittedRequest(...)`

The manager also builds a shard-local `ResDBConfig` before constructing this
commitment. That keeps PBFT quorum sizing based on the local shard instead of
the full network.

## Existing PBFT And 3PC Hooks

Several existing files were extended so the sharded wrappers could reuse the
old implementations instead of duplicating them.

PBFT `Commitment` now has virtual routing hooks:

```text
BroadcastConsensusMsg(...)
SendConsensusMsgToReplica(...)
SendResponseMsg(...)
```

Flat PBFT and flat 3PC still use the default `ReplicaCommunicator` behavior.
The sharded commitment classes override only the consensus-message hooks. Client
responses continue through the normal response path.

`Commitment3PC` now uses request metadata for coordinator identity when present.
That lets sharded 3PC use `global_coordinator_id`, while flat 3PC can still fall
back to the current primary.

`MessageManager` now supports an optional response filter. Flat PBFT and flat
3PC leave it unset. Sharded 3PC installs a filter that allows client responses
only when:

```text
request.coordinator_shard_id == local shard id
```

This lets every shard execute the transaction while only the coordinator shard
answers the proxy.

## Request Metadata

The shared request type is extended in:

```text
platform/proto/resdb.proto
```

The sharded fields are carried directly on `Request` so normal signing,
forwarding, recovery, and batching paths preserve the metadata:

- `global_txn_id`
- `coordinator_shard_id`
- `global_coordinator_id`
- `local_shard_id`
- `is_global_3pc`
- `is_local_shard_pbft`

The new request type:

```text
TYPE_SHARD_PBFT_NEW_TXN
```

marks the internal transition from global 3PC commit to local PBFT ordering.

## End-To-End Request Path

For a normal KV `set` or `get`, the path is:

```text
1. kv_service_tools sends a client request to the proxy/client node.
2. ShardedResponseManager batches the request.
3. ShardRouter chooses the next shard leader.
4. The proxy sends TYPE_NEW_TXNS to that leader and annotates sharded metadata.
5. The chosen shard leader becomes the global 3PC coordinator.
6. Shard3PCCommitment runs global 3PC among shard leaders.
7. On GLOBAL_COMMIT, each shard leader calls StartLocalPBFTFromGlobalCommit.
8. The request becomes TYPE_SHARD_PBFT_NEW_TXN, then local TYPE_PRE_PREPARE.
9. ShardPBFTCommitment runs PBFT inside the local shard.
10. MessageManager executes the ordered request.
11. Non-coordinator shards suppress client responses.
12. Coordinator-shard replicas send responses to the proxy.
13. ShardedResponseManager collects enough coordinator-shard responses.
14. The client receives the KV result.
```

## Log Inspection

The sharded trace can be inspected with:

```bash
grep -E "sharded_3pc_trace|shard_pbft_trace|sharded_handoff" server*.log
```

For the default `4 shards x 4 replicas` topology, global 3PC traces should be
most visible in the shard leader logs:

```text
server0.log   node 1
server4.log   node 5
server8.log   node 9
server12.log  node 13
```
