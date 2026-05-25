# Sharded 3PC/POE Execution Trace

This note gives a lightweight map of the sharded 3PC/POE implementation. It is
intended as a file-oriented trace of the execution path: how the POE service is
launched, how requests move through global 3PC, how they enter local POE, how
proofs and certificates control client-visible responses, and how local rollback
returns state to a checkpoint.

## High-Level Architecture

The POE mode keeps the same global sharded architecture as sharded 3PC/PBFT, but
replaces the local shard PBFT layer with Proof of Execution:

```text
client / proxy
  -> round-robin shard leader selection
  -> global 3PC across shard leaders
  -> local POE inside every shard
  -> optimistic execution on local shard replicas
  -> local proof collection by shard leader
  -> POE certificate broadcast inside the shard
  -> coordinator-shard-only response release
```

The PBFT/POE comparison:

- `kv_service_sharded_3pc` runs global 3PC plus local PBFT.
- `kv_service_sharded_3pc_poe` runs global 3PC plus local POE.

The two modes share the same shard metadata, proxy routing, sharded
communication, and global 3PC wrapper. The main difference is the local
consensus module installed after global 3PC commits.

## Startup And Configuration

The POE entrypoint is:

```text
service/kv/kv_service_sharded_3pc_poe.cpp
```

It mirrors `kv_service_sharded_3pc.cpp` and accepts the same arguments:

```text
kv_service_sharded_3pc_poe <server_config> <shard_config> <private_key> <cert_file>
```

The service calls:

```cpp
CustomGenerateResDBServerWithShardConfig<ConsensusManagerSharded3PCPOE>(...)
```

That factory path constructs the normal network-wide `ResDBConfig`, passes the
shard config path into the consensus manager, and preserves the existing KV
executor and storage setup.

The POE launch script is:

```text
service/tools/kv/server_tools/start_kv_service_sharded_3pc_poe.sh
```

It mirrors the sharded 3PC/PBFT launcher, but builds and runs:

```text
//service/kv:kv_service_sharded_3pc_poe
```

It uses the same generated files:

- `service/tools/config/server/server.config`
- `service/tools/config/interface/service.config`
- `service/tools/config/shard/shard.config`
- `service/tools/data/cert/node*.key.pri`
- `service/tools/data/cert/cert_*.cert`

The existing sharded config generator is reused:

```text
service/tools/kv/server_tools/generate_config_sharded.sh
```

That means sharded 3PC/PBFT and sharded 3PC/POE can be launched against the same
topology and credentials.

## Sharded POE Consensus Manager

The central dispatcher for POE mode is:

```text
platform/consensus/ordering/sharded_3pc/consensus_manager_sharded_3pc_poe.{h,cpp}
```

`ConsensusManagerSharded3PCPOE` inherits from `ConsensusManagerPBFT`, but defers
the base PBFT commitment, response manager, and recovery setup. It then installs
the sharded POE-specific pieces.

For a server replica, it creates:

- `ShardMetadata`
- a shard-local `ResDBConfig`
- `ShardCommunicator`
- `ShardPOECommitment` as the inherited local `commitment_`
- `Shard3PCCommitment` as `commitment_3pc_`

For a client/proxy node, it creates:

- `ShardMetadata`
- `ShardedResponseManager`

Client/proxy nodes do not own global 3PC or local POE commitments. They only
batch client requests and route them to shard leaders.

The dispatcher sends request types to the proper layer:

- `TYPE_CLIENT_REQUEST` and `TYPE_RESPONSE` go to the response manager.
- `TYPE_NEW_TXNS` enters global 3PC at the selected shard leader.
- `TYPE_3PC_*` messages go to `Shard3PCCommitment`.
- `TYPE_POE_EXECUTE`, `TYPE_POE_PROOF`, `TYPE_POE_CERT`, and
  `TYPE_POE_ROLLBACK` go to `ShardPOECommitment`.
- Local PBFT request types are intentionally not part of the POE manager path.

The manager also installs the coordinator-shard response filter:

```text
request.coordinator_shard_id == local shard id
```

All shards execute after global commit, but only replicas in the coordinator
shard may enqueue client/proxy responses.

## Proxy Routing

The POE path reuses the sharded proxy components:

```text
platform/consensus/ordering/sharded_3pc/shard_router.{h,cpp}
platform/consensus/ordering/sharded_3pc/sharded_response_manager.{h,cpp}
```

`ShardRouter` rotates through shard IDs and returns the next
`{shard_id, leader_id}` route.

`ShardedResponseManager` sends each batch to the chosen shard leader as
`TYPE_NEW_TXNS` and annotates the request with sharded metadata:

- `global_txn_id`
- `coordinator_shard_id`
- `global_coordinator_id`
- `is_global_3pc`

It also records the route for each local client request so timeout resends go to
the same coordinator shard. Client completion still uses the coordinator shard's
response threshold, not the full deployment.

## Global 3PC Layer

Global ordering is still handled by:

```text
platform/consensus/ordering/sharded_3pc/shard_3pc_commitment.{h,cpp}
```

`Shard3PCCommitment` inherits from the flat `Commitment3PC`. The existing 3PC
phase logic remains responsible for:

- prepare
- vote commit / vote abort
- precommit
- precommit ack
- global commit / global abort

The sharded wrapper changes only topology-sensitive behavior:

- global 3PC broadcasts go to shard leaders, not every replica;
- the expected 3PC participant count is the number of shards;
- directed 3PC sends use `ShardCommunicator`;
- global commit calls a callback instead of executing directly.

In POE mode, the callback is:

```cpp
ConsensusManagerSharded3PCPOE::StartLocalPOEFromGlobalCommit(...)
```

That callback creates the boundary between globally ordered 3PC traffic and
local shard POE traffic.

## Local POE Layer

The local POE module is:

```text
platform/consensus/ordering/sharded_3pc/shard_poe_commitment.{h,cpp}
```

`ShardPOECommitment` inherits from PBFT `Commitment` so it can reuse the
existing commitment infrastructure, duplicate manager, response thread, and
message manager integration. It overrides the consensus routing hooks so local
POE messages stay inside the local shard through `ShardCommunicator`.

The local POE messages are:

```text
TYPE_POE_EXECUTE
TYPE_POE_PROOF
TYPE_POE_CERT
TYPE_POE_ROLLBACK
```

### POE Execute

After global 3PC commits, each shard leader calls:

```cpp
ShardPOECommitment::ProcessGlobalCommittedRequest(...)
```

Only the local shard leader sends the local execute trigger. It converts the
globally committed request into `TYPE_POE_EXECUTE`, preserves the global
metadata, sets `local_shard_id`, marks `is_local_shard_poe`, and broadcasts to
the local shard.

Each replica validates `TYPE_POE_EXECUTE` in:

```cpp
ShardPOECommitment::ProcessPOEExecuteMsg(...)
ShardPOECommitment::ValidatePOEExecuteRequest(...)
```

Validation checks the signed context, sender identity, local shard target, POE
marker, sequence, hash, proxy id, and global coordinator metadata. Once valid,
the request is executed optimistically with:

```cpp
MessageManager::ExecuteOrderedRequest(...)
```

### POE Proof

POE proofs are produced through the generic post-execute hook added to
`MessageManager`:

```cpp
MessageManager::SetPostExecuteHook(...)
```

`ShardPOECommitment` installs this hook in its constructor. After local
execution produces a `BatchUserResponse`, the hook calls:

```cpp
ShardPOECommitment::SendPOEProof(...)
ShardPOECommitment::BuildPOEProofDigest(...)
ShardPOECommitment::BuildPOEResultDigest(...)
```

The proof digest binds:

- sequence number
- request hash
- global transaction id
- coordinator shard id
- local shard id
- digest of the ordered response payload bytes

The proof is sent as `TYPE_POE_PROOF` to the local shard leader. The response
payload digest is stored in `data`, the full proof digest is stored in
`data_hash`, and `data_signature` signs the proof digest.

### POE Certificate

Only the local shard leader collects proofs. Proof handling happens in:

```cpp
ShardPOECommitment::ProcessPOEProofMsg(...)
ShardPOECommitment::ValidatePOEProofRequest(...)
ShardPOECommitment::BuildPOECertRequest(...)
```

Proofs are grouped by transaction and proof digest. Duplicate proofs from the
same sender are ignored. A conflicting proof digest from the same sender
triggers local rollback.

When the leader collects enough matching proofs, it broadcasts `TYPE_POE_CERT`
to the local shard. The threshold comes from the shard-local config:

```text
config_.GetMinDataReceiveNum() == 2f + 1
```

The certificate carries:

- original transaction identity and sharded metadata
- result digest in `data`
- proof digest in `data_hash`
- matching proof signatures in `committed_certs`
- leader signature in `data_signature`

Replicas validate certificates in:

```cpp
ShardPOECommitment::ProcessPOECertMsg(...)
ShardPOECommitment::ValidatePOECertRequest(...)
```

After a valid cert, the replica releases the held response:

```cpp
MessageManager::ReleaseHeldResponse(seq, hash)
```

## Response Holding And Client Safety

POE has three important safety moments.

### Optimistic Execution

A replica executes after receiving one valid `TYPE_POE_EXECUTE` from its local
shard leader. This is fast, but still speculative.

### POE Certification

The leader certifies after collecting `2f + 1` matching local proofs. This is
the normal client-visible safety point. Responses are not queued to the proxy
until the corresponding `TYPE_POE_CERT` is valid.

This behavior is implemented by additions to:

```text
platform/consensus/ordering/pbft/message_manager.{h,cpp}
```

The important hooks are:

- `MessageManager::SetResponseHoldPredicate(...)`
- `MessageManager::ReleaseHeldResponse(...)`
- `MessageManager::DropHeldResponse(...)`
- `MessageManager::DropHeldResponsesAfter(...)`

`ShardPOECommitment` installs a hold predicate for local POE executes:

```text
request.type() == TYPE_POE_EXECUTE && request.is_local_shard_poe()
```

`MessageManager` stores the completed response until a valid certificate
releases it. Release still passes through the response filter, so
non-coordinator shards execute silently while coordinator-shard replicas respond
to the proxy.

### Checkpoint Stability

Checkpoint stability is stronger than POE certification. POE certification is
the normal response point, but a transaction is rollback-immune only once the
checkpoint system has stabilized a checkpoint at or beyond that transaction.

## Rollback Path

Minimal POE rollback is local-shard rollback to a stable checkpoint.

Rollback is represented by:

```text
TYPE_POE_ROLLBACK
```

The local shard leader builds rollback messages with:

```cpp
ShardPOECommitment::BuildPOERollbackRequest(...)
ShardPOECommitment::BroadcastPOERollback(...)
```

Followers validate rollback messages in:

```cpp
ShardPOECommitment::ValidatePOERollbackRequest(...)
ShardPOECommitment::ProcessPOERollbackMsg(...)
```

The rollback request uses `request.seq()` as the checkpoint boundary. The
rollback digest binds:

- checkpoint sequence
- local shard id
- reason string

The current automatic rollback trigger is a concrete local proof conflict:

```text
same local replica, same transaction, different proof digest
```

When this happens, the leader rolls back to:

```cpp
MessageManager::GetStableCheckpoint()
```

and broadcasts the rollback to the local shard.

Rollback coordination is implemented across several layers:

- `MessageManager::RollbackToCheckpoint(...)`
  - drops held POE responses after the checkpoint
  - calls executor rollback
  - resets ordering sequence state
  - resets collector and checkpoint manager state
- `TransactionExecutor::RollbackToCheckpoint(...)`
  - clears queued/cached speculative work
  - resets execution sequence tracking
- `TransactionManager::RollbackToCheckpoint(...)`
  - forwards rollback to storage
  - resets the transaction manager sequence cursor
- `MemoryDB::RollbackToCheckpoint(...)`
  - trims in-memory per-key sequence histories
- `ResLevelDB::RollbackToCheckpoint(...)`
  - truncates persisted `ValueHistory` records
  - deletes keys created only after the checkpoint
  - flushes the block cache

This is not a full global rollback agreement. It is local rollback machinery
for the POE path, with a concrete local trigger for conflicting proof digests.

## Existing Hooks Added For POE

Several existing PBFT-oriented components were extended so POE can reuse them.

### Request Proto

`platform/proto/resdb.proto` adds:

```text
TYPE_POE_EXECUTE
TYPE_POE_PROOF
TYPE_POE_CERT
TYPE_POE_ROLLBACK
is_local_shard_poe
```

The existing sharded metadata fields are reused:

- `global_txn_id`
- `coordinator_shard_id`
- `global_coordinator_id`
- `local_shard_id`
- `is_global_3pc`

### Message Manager

`MessageManager` now supports:

- response filters for coordinator-shard-only replies;
- post-execute hooks for POE proof generation;
- response holding so optimistic execution does not immediately reach clients;
- release/drop handling for POE certificates and rollback;
- rollback coordination to reset local ordering and execution state.

Flat PBFT, flat 3PC, and sharded 3PC/PBFT leave the POE-specific hooks unset.

### Storage And Execution

Rollback APIs were added to:

```text
chain/storage/storage.h
chain/storage/memory_db.{h,cpp}
chain/storage/leveldb.{h,cpp}
executor/common/transaction_manager.{h,cpp}
platform/consensus/execution/transaction_executor.{h,cpp}
```

These APIs are used by POE rollback to restore KV state and local executor
state to a checkpoint boundary.

## End-To-End Request Path

For a normal KV `set` or `get` in sharded 3PC/POE mode, the path is:

```text
1. kv_service_tools sends a client request to the proxy/client node.
2. ShardedResponseManager batches the request.
3. ShardRouter chooses the next shard leader.
4. The proxy sends TYPE_NEW_TXNS to that leader with sharded metadata.
5. The chosen shard leader becomes the global 3PC coordinator.
6. Shard3PCCommitment runs global 3PC among shard leaders.
7. On GLOBAL_COMMIT, each shard leader calls StartLocalPOEFromGlobalCommit.
8. The request becomes TYPE_POE_EXECUTE for that local shard.
9. ShardPOECommitment broadcasts TYPE_POE_EXECUTE to local shard replicas.
10. Each local replica validates and optimistically executes the request.
11. MessageManager holds the response and calls the post-execute hook.
12. ShardPOECommitment sends TYPE_POE_PROOF to the local shard leader.
13. The leader collects 2f + 1 matching proofs.
14. The leader broadcasts TYPE_POE_CERT to the local shard.
15. Replicas validate the cert and release held responses.
16. Non-coordinator shards suppress responses through the response filter.
17. Coordinator-shard replicas send responses to the proxy.
18. ShardedResponseManager collects enough coordinator-shard responses.
19. The client receives the KV result.
```

If a local proof conflict is detected before certification, the shard leader
broadcasts `TYPE_POE_ROLLBACK`, and local state is restored to the stable
checkpoint.

## Log Inspection

Global 3PC and local POE traces can be inspected with:

```bash
grep -E "sharded_3pc_trace|shard_poe_trace|sharded_poe_handoff|POE cert|POE rollback" server*.log
```

Useful trace markers:

- `[sharded_3pc_trace]` shows global 3PC traffic among shard leaders.
- `[sharded_poe_handoff]` shows global commit entering local POE.
- `[shard_poe_trace]` shows local POE execute, proof, cert, release, and
  rollback behavior.

For the default `4 shards x 4 replicas` topology, global 3PC traces should be
most visible in shard leader logs:

```text
server0.log   node 1
server4.log   node 5
server8.log   node 9
server12.log  node 13
```

Local POE traces should appear in every shard's server logs after each global
commit.

## Comparison With Sharded 3PC/PBFT

The baseline sharded 3PC/PBFT path uses local PBFT after global 3PC:

```text
GLOBAL_COMMIT -> TYPE_SHARD_PBFT_NEW_TXN -> PRE_PREPARE -> PREPARE -> COMMIT
```

The POE path replaces that local PBFT sequence with:

```text
GLOBAL_COMMIT -> TYPE_POE_EXECUTE -> optimistic execution
              -> TYPE_POE_PROOF -> TYPE_POE_CERT -> response release
```

The intended comparison is:

- sharded 3PC/PBFT keeps local PBFT's prepare and commit phases;
- sharded 3PC/POE collapses the local phases into optimistic execution plus
  proof certification;
- both modes keep global 3PC, shard-aware routing, and coordinator-shard-only
  client responses;
- only the POE mode uses response holding, POE certificates, and rollback to
  checkpoint.

## Current Limitations

Global rollback agreement has not been implemented. The current rollback path is
local to one shard, so a shard-level rollback after other shards have continued
could create cross-shard consistency problems.

Proof timeout handling has also not been implemented. If enough matching
`TYPE_POE_PROOF` messages never arrive, there is no timer-driven rollback or
recovery path yet.

These pieces are left out for now because the assignment assumes failures will
not occur during the tested execution path.
