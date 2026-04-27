# ResilientDB 3PC Implementation Notes

## Purpose of This Document

This document explains the design, motivation, and file-level implementation of the Three-Phase Commit (3PC) protocol added to ResilientDB. This document is written as implementation documentation rather than a theoretical protocol explanation.

---

## Project Goal

The goal of the change set is to replace ResilientDB's default PBFT consensus/commit path with a 3PC-style coordinator/participant protocol while preserving as much of the existing ResilientDB architecture as possible.

The implementation assumes the following assignment constraints:

- Default ResilientDB deployment: **4 replicas + 1 proxy/client-facing node**.
- The replica selected by the proxy acts as the **single coordinator** for all transactions.
- Every transaction involves **all 4 replicas**.
- No concurrency control is required.
- Transactions may be assumed to commit in the common path.
- Existing execution and reply-to-client steps should remain intact after the commit protocol completes.

---

## High-Level Design Choice

### Why 3PC was implemented as a new protocol module

ResilientDB already contains protocol-specific ordering directories such as `pbft/` and `geo_pbft/`. The chosen design follows that pattern.

Instead of rewriting the entire database stack, the implementation introduces a new protocol-specific ordering module:

- `consensus_manager_3pc.h/.cpp`
- `commitment_3pc.h/.cpp`

This allows the system to keep using the following working ResilientDB infrastructure:

- network ingress in `ConsensusManager`
- KV client and server startup paths
- proxy/client batching and reply path
- storage and execution logic
- duplicate detection
- sequence assignment
- recovery scaffolding
- checkpoint/message infrastructure where still useful

### Core architectural principle

The implementation changes **the protocol state machine**, not the entire service architecture.

The idea is:

- keep the **client/proxy → request ingress → execution → reply** skeleton
- replace the PBFT-specific ordering path:
  - `PRE_PREPARE`
  - `PREPARE`
  - `COMMIT`
- with a 3PC-specific path:
  - `PREPARE`
  - `VOTE_COMMIT` / `VOTE_ABORT`
  - `PRECOMMIT`
  - `PRECOMMIT_ACK`
  - `GLOBAL_COMMIT` / `GLOBAL_ABORT`

---

## Main Design Motivation

### Why PBFT scaffolding was reused

PBFT's ResilientDB implementation already solves a large amount of non-protocol work:

- request batching
- request forwarding
- signed message transport
- duplicate management
- sequence allocation
- post-execution response plumbing
- recovery log loading
- general server startup

These features are useful even if PBFT itself is removed.

### Why PBFT's commit logic could not be reused directly

PBFT's `MessageManager` and `Commitment` logic are built around a **PBFT quorum-based collector model**:

- collect `PRE_PREPARE`
- collect enough `PREPARE`
- collect enough `COMMIT`
- transition to execution when PBFT quorum conditions are met

3PC has different semantics:

- the **coordinator** decides phase transitions
- **participants** do not locally infer commit from a PBFT-style quorum
- commit is finalized only after the coordinator sends `GLOBAL_COMMIT`

Therefore:

- **sequence numbering and response plumbing** can be reused
- **PBFT commit/collector semantics** cannot be reused unchanged

This is why a new `Commitment3PC` was introduced and why `MessageManager` was extended with a protocol-neutral execution hook.

---

## Files Added or Modified

## 1. `platform/consensus/ordering/three_pc/consensus_manager_3pc.h/.cpp`

### Added methods

- constructor / destructor
- `InitRecovery3PC`
- `ConsensusCommit`
- `InternalConsensusCommit3PC`
- `GetCommitment3PC`

### Role in the implementation

This is the protocol-level dispatcher for 3PC.

It reuses the broader PBFT consensus-manager scaffolding but swaps the actual ordering/commit protocol handlers.

### Main responsibilities

- keep existing request ingress behavior for:
  - `TYPE_CLIENT_REQUEST`
  - `TYPE_RESPONSE`
  - query/recovery paths
- dispatch `TYPE_NEW_TXNS` into coordinator-side 3PC startup
- dispatch 3PC protocol messages to `Commitment3PC`
- preserve as much of the generic consensus-manager skeleton as possible
- initialize recovery so that replay goes through **3PC dispatch**, not PBFT dispatch

### Important design decision

`ConsensusManager3PC` reuses most PBFT consensus-manager construction, but the protocol-specific dispatch is replaced with 3PC handlers.

`GetCommitment3PC()` exists because `commitment_` is stored as a base-class pointer (`Commitment`), while 3PC-specific handlers such as `RecordVoteCommit(...)` only exist on `Commitment3PC`.

---

## 2. `platform/consensus/ordering/three_pc/commitment_3pc.h/.cpp`

### Added methods

- constructor
- `ProcessNewRequest`
- `ProcessPrepareMsg`
- `RecordVoteCommit`
- `ProcessVoteAbortMsg`
- `ProcessPreCommitMsg`
- `ProcessPreCommitAckMsg`
- `ProcessGlobalCommitMsg`
- `ProcessGlobalAbortMsg`
- `IsCoordinator`
- `CoordinatorId`
- `MaybeBroadcastPreCommit`
- `MaybeBroadcastGlobalCommit`
- `ExecuteCommittedTxn`

### Added data structures

- `enum class ThreePCPhase`
- `struct ThreePCTxnState`
- `txn_mu_`
- `txn_state_`

### Role in the implementation

This file is the core 3PC state machine.

It replaces PBFT's `Commitment` stage logic with explicit coordinator/participant state transitions.

### Main responsibilities

#### `ProcessNewRequest`
Coordinator-side entry point for a new transaction.

Responsibilities:
- reject malformed requests
- reject already executed requests
- forward to coordinator if the current node is not the coordinator
- verify original client payload signature
- check user pre-verification hook if present
- prevent duplicate proposals
- allocate a sequence number using `MessageManager::AssignNextSeq()`
- create local transaction state
- record the coordinator's own implicit commit vote
- broadcast `TYPE_3PC_PREPARE`

This is the direct 3PC replacement for PBFT's `Commitment::ProcessNewRequest(...)`, whose old job was to build and broadcast `TYPE_PRE_PREPARE`.

#### `ProcessPrepareMsg`
Participant-side handler for `TYPE_3PC_PREPARE`.

Responsibilities:
- verify request origin and signature
- store transaction as locally `READY`
- retain the original request payload
- send `TYPE_3PC_VOTE_COMMIT` to the coordinator

#### `RecordVoteCommit`
Coordinator-side handler for incoming participant commit votes.

Responsibilities:
- update vote-tracking state for the transaction
- call `MaybeBroadcastPreCommit(...)`

#### `ProcessVoteAbortMsg`
Coordinator-side abort path.

Responsibilities:
- transition local transaction state to `ABORT`
- erase proposed duplicate state
- broadcast `TYPE_3PC_GLOBAL_ABORT`

Even though the common-case project assumption is that transactions commit, the abort path exists so that the protocol remains structurally valid 3PC.

#### `ProcessPreCommitMsg`
Participant-side handler for coordinator `PRECOMMIT`.

Responsibilities:
- transition local transaction state to `PRECOMMIT`
- send `TYPE_3PC_PRECOMMIT_ACK` to coordinator

#### `ProcessPreCommitAckMsg`
Coordinator-side handler for participant precommit acknowledgements.

Responsibilities:
- update precommit-ack tracking
- call `MaybeBroadcastGlobalCommit(...)`

#### `ProcessGlobalCommitMsg`
Participant-side final commit handler.

Responsibilities:
- transition local state to `COMMIT`
- remove duplicate proposed state
- trigger execution via `ExecuteCommittedTxn(...)`

#### `ProcessGlobalAbortMsg`
Participant-side abort-finalization handler.

Responsibilities:
- transition local transaction state to `ABORT`
- clear proposed duplicate state

#### `MaybeBroadcastPreCommit`
Coordinator-side transition guard.

Responsibilities:
- check whether enough commit votes have arrived
- if so, transition from `READY` to `PRECOMMIT`
- broadcast `TYPE_3PC_PRECOMMIT`

This function is intentionally safe to call many times. Most calls are no-ops until the vote condition is satisfied.

#### `MaybeBroadcastGlobalCommit`
Coordinator-side transition guard.

Responsibilities:
- check whether enough precommit acknowledgements have arrived
- if so, transition to `COMMIT`
- broadcast `TYPE_3PC_GLOBAL_COMMIT`
- execute the committed transaction locally as coordinator

#### `ExecuteCommittedTxn`
Bridges the 3PC protocol into ResilientDB's normal execution pipeline.

Responsibilities:
- build an execution-ready request object
- stamp final sequence/view/primary-coordinator metadata
- call `MessageManager::ExecuteOrderedRequest(...)`

### Added state tracking

#### `ThreePCPhase`
Tracks per-transaction protocol phase:

- `kInitial`
- `kReady`
- `kPreCommit`
- `kCommit`
- `kAbort`

#### `ThreePCTxnState`
Stores protocol metadata for one transaction:

- `seq`
- `phase`
- `hash`
- `coordinator_id`
- `proxy_id`
- original request payload
- participant commit votes received
- participant precommit acknowledgements received

#### `txn_mu_`
Mutex protecting `txn_state_`.

#### `txn_state_`
Map from sequence number to local 3PC transaction state.

Important note: each node has its **own local** `txn_state_`. When `GLOBAL_COMMIT` or `GLOBAL_ABORT` is broadcast, every node updates its own local state map. They are not writing the same in-memory object.

---

## 3. `platform/consensus/ordering/pbft/message_manager.h/.cpp`

### Added method

- `ExecuteOrderedRequest`

### Why this change was required

The current PBFT `MessageManager` only exposes PBFT-style commit/execution flow through `AddConsensusMsg(...)`, which assumes:

- `PRE_PREPARE`
- `PREPARE`
- `COMMIT`
- PBFT collector state transitions

3PC does not reach execution through those PBFT transitions.

Once `GLOBAL_COMMIT` is final in 3PC, the request should execute directly.

Therefore, `ExecuteOrderedRequest(...)` was added as a protocol-neutral execution hook.

### Responsibilities of `ExecuteOrderedRequest`

- validate the ordered request
- avoid re-executing an already committed sequence if applicable
- mark the sequence committed in checkpoint/commit state
- call the transaction executor directly
- allow the existing response queue / reply path to remain unchanged

### Motivation

This method allows 3PC to reuse ResilientDB's existing execution and reply infrastructure without pretending that 3PC went through PBFT's collector/commit transitions.

---

## 4. `resdb.proto`

### Added request types

Seven 3PC request types were added to the protobuf enum used by `Request::type()`:

- `TYPE_3PC_PREPARE`
- `TYPE_3PC_VOTE_COMMIT`
- `TYPE_3PC_VOTE_ABORT`
- `TYPE_3PC_PRECOMMIT`
- `TYPE_3PC_PRECOMMIT_ACK`
- `TYPE_3PC_GLOBAL_COMMIT`
- `TYPE_3PC_GLOBAL_ABORT`

### Why this change was required

PBFT message types are protocol-specific and cannot be safely overloaded for 3PC.

A legitimate 3PC implementation requires explicit message-type separation so that:

- replay and logging are interpretable
- dispatch logic is correct
- protocol transitions are not confused with PBFT transitions

---

## 5. `platform/consensus/ordering/pbft/consensus_manager_pbft.h/.cpp`

### Added / changed methods

- `InitRecoveryPBFT`
- constructor / initialization change adding `defer_recovery_init`

### Why this change was required

`ConsensusManager3PC` inherits PBFT scaffolding. However, the original PBFT constructor immediately invoked recovery replay using PBFT's internal dispatch path.

That is a problem for 3PC, because constructor order means:

1. `ConsensusManagerPBFT` constructor runs first
2. PBFT recovery replay happens there
3. only then does `ConsensusManager3PC` constructor body run

If recovery is not deferred, recovered log entries are replayed through PBFT before the 3PC manager is fully installed.

### What `defer_recovery_init` does

The constructor now takes a flag, `defer_recovery_init`.

- if `false`, normal PBFT behavior remains intact and `InitRecoveryPBFT()` is called
- if `true`, PBFT recovery initialization is skipped so that a derived protocol manager such as `ConsensusManager3PC` can install its own recovery replay callback

### What `InitRecoveryPBFT` does

This isolates PBFT recovery replay into a dedicated function instead of hardwiring it into the constructor body.

This makes it possible for `ConsensusManager3PC` to implement `InitRecovery3PC()` and replay logs through `InternalConsensusCommit3PC(...)` instead.

---

## Protocol Flow Implemented

## Normal request path

1. Client/proxy submits a transaction.
2. `TYPE_NEW_TXNS` reaches `ConsensusManager3PC`.
3. `Commitment3PC::ProcessNewRequest(...)` runs at the coordinator.
4. Coordinator assigns sequence and broadcasts `TYPE_3PC_PREPARE`.
5. Participants handle `TYPE_3PC_PREPARE` in `ProcessPrepareMsg(...)` and reply with `TYPE_3PC_VOTE_COMMIT`.
6. Coordinator records votes via `RecordVoteCommit(...)`.
7. Once all required votes arrive, coordinator broadcasts `TYPE_3PC_PRECOMMIT`.
8. Participants handle `TYPE_3PC_PRECOMMIT` and reply with `TYPE_3PC_PRECOMMIT_ACK`.
9. Coordinator collects ACKs via `ProcessPreCommitAckMsg(...)`.
10. Once all required ACKs arrive, coordinator broadcasts `TYPE_3PC_GLOBAL_COMMIT`.
11. Participants handle `TYPE_3PC_GLOBAL_COMMIT` and execute via `ExecuteCommittedTxn(...)`.
12. Coordinator also executes via `ExecuteCommittedTxn(...)`.
13. Existing execution/reply path returns responses to the client.

## Abort path

Abort support exists structurally:

- participant may send `TYPE_3PC_VOTE_ABORT`
- coordinator handles it in `ProcessVoteAbortMsg(...)`
- coordinator broadcasts `TYPE_3PC_GLOBAL_ABORT`
- participants handle it in `ProcessGlobalAbortMsg(...)`

Even if the expected workload is always-commit, the presence of the abort path helps preserve 3PC protocol correctness.

---

## Recovery Design

## Why recovery required special handling

A naive derived constructor that simply calls `recovery_->ReadLogs(...)` again is not sufficient if the PBFT base constructor already performed recovery.

That would cause:

- one PBFT replay pass
- then one 3PC replay pass

The fix is to defer PBFT recovery initialization in the base class and let `ConsensusManager3PC` install its own replay lambda.

## What `InitRecovery3PC` should do

`InitRecovery3PC()` mirrors PBFT recovery initialization but replays recovered requests using:

- `InternalConsensusCommit3PC(...)`

instead of:

- `InternalConsensusCommit(...)`

This ensures that any recovered protocol messages are interpreted using 3PC logic.

---

## Performance and Benchmarking Design Choice

## Why PBFT `PerformanceManager` is not reused

PBFT performance mode is not just a benchmarking wrapper. When performance mode is enabled, ResilientDB swaps `ResponseManager` out and uses `PerformanceManager`, which generates PBFT-oriented benchmark traffic and PBFT-specific assumptions.

That is not protocol-compatible with 3PC.

Examples of PBFT-specific assumptions in performance mode include:

- primary-oriented routing semantics
- PBFT timeout/resend assumptions
- PBFT-specific benchmark counters such as `max txn` values that are not true throughput

## Chosen design choice

The intended 3PC measurement approach is:

- run the system in **normal KV mode**
- allow it to use the normal `ResponseManager`
- measure throughput and latency using an **external script** that issues requests and times responses

This avoids mixing PBFT-specific benchmark internals into the 3PC protocol evaluation.

## Why this is preferable

This approach ensures that only the consensus protocol changed, while the rest of the working system remains intact.

---

## Important Implementation Invariants


## Protocol correctness / dispatch

- `TYPE_NEW_TXNS` is routed into `Commitment3PC::ProcessNewRequest(...)`, not PBFT commit logic.
- PBFT protocol message types are not used to drive 3PC commit transitions.
- All seven 3PC request types exist in `resdb.proto` and are handled in `ConsensusManager3PC`.

## Coordinator/participant separation

- only the coordinator broadcasts `TYPE_3PC_PREPARE`
- only participants send `TYPE_3PC_VOTE_COMMIT` / `TYPE_3PC_VOTE_ABORT`
- only the coordinator broadcasts `TYPE_3PC_PRECOMMIT`
- only participants send `TYPE_3PC_PRECOMMIT_ACK`
- only the coordinator broadcasts `TYPE_3PC_GLOBAL_COMMIT` / `TYPE_3PC_GLOBAL_ABORT`

## Transaction state management

- `txn_state_` exists and is protected by `txn_mu_`
- phase transitions follow legal 3PC ordering:
  - `INITIAL -> READY -> PRECOMMIT -> COMMIT`
  - abort path is possible where appropriate
- duplicate votes / duplicate ACKs are ignored safely

## Execution path

- execution occurs only after `GLOBAL_COMMIT`
- 3PC execution uses `MessageManager::ExecuteOrderedRequest(...)`
- normal response handling after execution is still intact

## Recovery

- PBFT recovery initialization is deferrable in the base class
- `ConsensusManager3PC` replay goes through `InternalConsensusCommit3PC(...)`
- recovered log entries are not first replayed through PBFT

## Non-consensus reuse

- client request ingress is still reused where possible
- execution/reply path is unchanged outside the protocol boundary
- performance mode is not falsely claimed to be 3PC-compatible unless a separate `PerformanceManager3PC` is actually implemented

---

## Known Caveats / Review Notes

1. `ConsensusManager3PC` may inherit substantial PBFT scaffolding. Review should confirm that PBFT-specific message dispatch and recovery assumptions have been neutralized where necessary.

2. If `PerformanceManager3PC` was not implemented, throughput and latency measurement should come from normal-mode external measurement, not PBFT performance benchmark output.

3. If the coordinator ignores its own broadcasted request states.

---


