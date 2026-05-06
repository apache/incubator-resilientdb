# RoK Native Token (Smart-Contract Path Only) — Updated Implementation Plan (2026-05)

This plan refreshes and tightens the earlier RoK-native-token plan against the **current ResilientDB codebase** in this repo. It is written to be directly implementable as a sequence of PR-sized milestones.

---

## 0) What’s true in the codebase today (verified)

### Unified KV service + contract requests

- The **unified KV service** runs `KVExecutor` (see `service/kv/kv_service.cpp`), which creates a `ContractTransactionManager` using the **same** `Storage*`.
- Contract RPCs are tunneled through KV via `KVRequest.smart_contract_request`:
  - `interface/kv/contract_client.cpp` serializes `resdb.contract.Request` into `KVRequest.smart_contract_request`.
  - `executor/kv/kv_executor.cpp` detects `!kv_request.smart_contract_request().empty()` and dispatches to `contract_manager_->ExecuteData(...)`.

### Contract commands that exist today

`proto/contract/rpc.proto` currently defines:

- `CREATE_ACCOUNT`
- `DEPLOY`
- `EXECUTE`
- `GETBALANCE`
- `SETBALANCE`

There is **no** native transfer command yet, and there is **no** RPC for “AddExternalAddress” even though `AddressManager::AddExternalAddress` exists.

### Balances & EVM execution today

- Persisted balance keys are stored in `GlobalState::{GetBalance,SetBalance}` as:
  - `contract_balance_<hex(keccak(address))>`
  - implemented in `executor/contract/manager/global_state.cpp`.
- EVM execution uses eEVM via:
  - `executor/contract/manager/contract_manager.cpp` → `eevm::Processor::run(..., /*value=*/0u, ...)`
- **Important mismatch (must be fixed for “native token semantics”):**
  - `GlobalState::get(addr)` returns an in-memory `SimpleAccount` and if missing does `create(addr, 0, {})`.
  - That means **EVM-visible balances are currently zero-initialized**, and do not reflect what `GETBALANCE/SETBALANCE` store in `Storage`.

### Accounts today are memory-only (must be fixed)

- `AddressManager` keeps `std::set<Address> users_` only in memory (`executor/contract/manager/address_manager.{h,cpp}`).
- `ContractTransactionManager` constructs `AddressManager()` fresh at process start (`executor/contract/executor/contract_executor.cpp`).
- Result: after restart/crash, previously created accounts are “forgotten” and callers fail `Exist(...)` checks in `Deploy()` / `Execute()`.

---

## 1) Scope & goals (explicit)

### In scope

- RoK exists **only** in the smart-contract request tunnel (unified path):
  - `CREATE_ACCOUNT`, `DEPLOY`, `EXECUTE`, `GETBALANCE`, `SETBALANCE` (legacy), plus a new `TRANSFER_ROK`.
- Persist “account existence” so accounts survive restarts/crashes.
- Make **a single source of truth for balances**, so:
  - `GETBALANCE`
  - `TRANSFER_ROK`
  - EVM execution (contract calls)
  all refer to the **same stored balance**.

### Out of scope (for this plan)

- KV request fees/payer integration.
- Full Ethereum compatibility (nonces, replay protection, signature validation).
- Cross-service token integration outside unified KV path.

---

## 2) Key design decisions (recommended)

### 2.0 Single `Storage` / same LevelDB (decided)

**Decision:** Keep **one** `Storage` instance for everything the unified KV service already persists:

- User KV (`SET` / `GET`, etc., via `KVExecutor`)
- Account registry (planned: `state/account/...` keys)
- RoK balances (`contract_balance_...` via `GlobalState`)
- Contract slot storage (`GlobalView` hex keys)

When the node is built with LevelDB, that is **the same on-disk LevelDB** (same DB directory the service uses today). There is **no** separate chain-state database in this phase.

**Why this is enough:** Logical separation comes from **key prefixes** (`state/account/`, `contract_balance_`, user-chosen keys). A **later** optional hardening is to reject user `SET` on reserved prefixes, or to split into two `Storage` instances for operations — **out of scope until needed**.

### 2.1 State key namespace

To avoid collisions with user KV keys and keep future migrations simple, introduce a dedicated prefix for RoK/account state:

- **Accounts**: `state/account/<address_hex>` → `"1"`
- **Balances**: keep the existing `contract_balance_...` keys for now (backward compatible), but treat them as **RoK balances**.

We can optionally migrate balances later to a `state/balance/...` namespace, but that is **not required** for viability.

### 2.2 Address string format

Use the existing client/server hex format already used in this repo:

- Addresses are passed as hex strings and converted by:
  - `AddressManager::AddressToHex(...)` / `HexToAddress(...)`

For account persistence keys, store the **client-facing address hex** exactly as accepted by `HexToAddress(...)` to avoid ambiguity.

### 2.3 Transfer semantics

Implement native transfer as a single contract command executed atomically in one consensus-committed request:

- Validate:
  - `from` account exists
  - `to` account exists (or decide to auto-create; recommended: **require exists** initially)
  - `amount > 0`
  - `from_balance >= amount`
- Apply:
  - `from_balance -= amount`
  - `to_balance += amount`

Overflow/underflow is handled by uint256 checks (use `uint256_t` / `eevm::to_uint256` conversions consistently).

### 2.4 SETBALANCE policy

Today `SETBALANCE` is effectively “admin overwrite” with no authorization. For RoK, keep it temporarily for testing/dev tooling but plan to restrict it later:

- Phase 1: keep as-is, clearly marked “unsafe/admin”.
- Phase 2: gate behind a configured “system/admin address” (or remove from public client surfaces).

---

## 3) Milestone plan (PR-sized, step-by-step)

Each milestone below is meant to be implementable and testable independently.

### Milestone A — Persist accounts (restart-safe)

**Goal**: `CREATE_ACCOUNT` results survive process restart; `Deploy/Execute` do not fail due to lost `users_` set.

**Proposed changes**

1. **Introduce account storage keys**
   - Add helper constants/functions (choose one location):
     - `executor/contract/manager/address_manager.*`, or
     - `executor/contract/executor/contract_executor.*`
   - Key format: `state/account/<address_hex>`

2. **Write-on-create**
   - When `CREATE_ACCOUNT` succeeds, persist:
     - `storage->SetValue("state/account/<addr_hex>", "1")`
   - Also insert into in-memory `users_` as today.

3. **Load-on-start**
   - In `ContractTransactionManager` constructor:
     - call `storage->GetKeyRange("state/account/", "state/account/~")`
     - for each key, parse `<address_hex>` suffix and insert into `AddressManager::users_`
   - This requires either:
     - (recommended) **inject `Storage*` into `AddressManager`**, or
     - add a `LoadFromStorage(Storage*)` method on `AddressManager`.

**Files likely touched**

- `executor/contract/executor/contract_executor.{h,cpp}`
- `executor/contract/manager/address_manager.{h,cpp}`
- (optional) add a small shared utility header for key formats.

**Acceptance checks**

- Create account → restart node → Deploy/Execute with that address works (no “Account not exist”).
- Startup does not scan all KV keys (only the `state/account/` range).

---

### Milestone B — Add `TRANSFER_ROK` to protobuf + executor

**Goal**: first-class atomic native transfer command.

**Proposed protobuf**

In `proto/contract/rpc.proto`, add:

- `TRANSFER_ROK = 6;`
- Fields (minimal, explicit):
  - `optional string from_account = 8;`
  - `optional string to_account = 9;`
  - `optional string amount = 10; // hex uint256 string`

Keep existing fields unchanged for backward compatibility.

**Executor changes**

1. Extend `ContractTransactionManager::ExecuteData` dispatch:
   - handle `TRANSFER_ROK`
2. Implement `TransferRoK(request)`:
   - parse addresses using `AddressManager::HexToAddress`
   - parse amount using `eevm::to_uint256(request.amount())`
   - read balances from `ContractManager::GetBalance(...)`
   - update balances via `ContractManager::SetBalance(...)`

**Important implementation note**

`GlobalState::GetBalance` currently returns a string from storage. If missing, it returns empty string.

For transfer, treat empty as **0**:

- `balance_str.empty() ? 0 : eevm::to_uint256(balance_str)`

**Files likely touched**

- `proto/contract/rpc.proto` (+ regenerate pb via Bazel build)
- `executor/contract/executor/contract_executor.{h,cpp}`
- `interface/kv/contract_client.{h,cpp}` (add `TransferRoK(...)`)

**Acceptance checks**

- Two accounts A,B:
  - set initial balance (temporary via `SETBALANCE`)
  - transfer amount
  - GETBALANCE reflects debit/credit correctly
- Negative cases:
  - insufficient funds
  - missing/invalid amount
  - non-existent accounts (per chosen policy)

---

### Milestone C — Make EVM-visible balance consistent with stored RoK balance

**Goal**: contracts “see” the same RoK balance that GETBALANCE/TRANSFER_ROK manage.

**Minimum viable fix (recommended)**

Update `GlobalState::get(addr)` behavior:

- On first access of an address not in `accounts` map:
  - load persisted RoK balance from storage (via `GetBalance(addr)`)
  - create `SimpleAccount(addr, persisted_balance, {})`

This ensures eEVM’s `Processor` sees correct balances when it touches accounts.

**Where to implement**

- `executor/contract/manager/global_state.cpp`:
  - change `GlobalState::get` and/or `create` to initialize accounts from storage.

**Acceptance checks**

- After `SETBALANCE` or `TRANSFER_ROK`, executing a contract that reads its own balance (or msg.sender balance, depending on eEVM exposure) observes the updated value (as supported by your contract patterns).

---

### Milestone D — Lock down “admin-only” balance mutation (optional but strongly recommended)

**Goal**: prevent arbitrary clients from setting anyone’s RoK balance.

Options (pick one):

- **D1**: Restrict `SETBALANCE` to a configured admin address:
  - add `caller_address` requirement for SETBALANCE
  - check it matches config (or a hardcoded system address for now)
- **D2**: Remove/disable SETBALANCE in production builds; keep only for local dev.

**Acceptance checks**

- Non-admin cannot call SETBALANCE successfully.

---

### Milestone E — Ecosystem surfaces (CLI / GraphQL / Wallet) (follow-on)

This is repo-dependent; implement once core state+transfer is stable.

- Add `transfer-rok` to whichever CLI is used in your environment.
- Expose transfer + balance in GraphQL service (if it currently wraps contract_client).
- Update ResVault UI to show RoK and use TRANSFER_ROK for sends.

---

## 4) Implementation notes / pitfalls (based on current code)

- **No AddExternalAddress RPC**: if you need importing existing addresses (wallet-generated), add an explicit command (e.g. `IMPORT_ACCOUNT`) rather than relying on the unused method.
- **Atomicity**: today, storage writes are simple `SetValue` calls. TRANSFER_ROK is “atomic” at the *logical transaction* level (single consensus request), but not as a multi-key DB transaction. This is still consistent under deterministic execution, but be careful if you introduce concurrency later.
- **Key derivation for balances**: balance keys are `contract_balance_<keccak(address)>` not `contract_balance_<address>`. Keep this stable unless you migrate with a versioning plan.
- **Parsing empty balances**: treat missing storage keys as zero to avoid `to_uint256("")` failures.

---

## 5) Concrete “start implementing now” action checklist

If you want the shortest path to an end-to-end demo:

1. **Milestone A** (persist accounts) — unblock restarts.
2. **Milestone B** (TRANSFER_ROK) — native sends.
3. **Milestone C** (EVM balance consistency) — contracts see RoK.
4. Add a tiny scripted test using `ContractClient`:
   - create A/B
   - set initial balance (until D)
   - transfer
   - get balances
   - restart server (manual)
   - deploy/execute using A to confirm account persists

