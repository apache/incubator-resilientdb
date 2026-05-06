# RoK Native Token: What Changed + What To Test

Branch: `feature/rok-native-token`  
Commit: `6ea92a968` (“Making accounts persistent”)

This doc is a quick, operator-friendly summary of **what the code now does** and **how to validate it** end-to-end.

---

## 1) What is implemented

### A. Accounts survive restart (persistence)

**Problem before:** `AddressManager` stored “known accounts” only in RAM (`users_`). After restart, previously created accounts were forgotten and `DEPLOY` / `EXECUTE` would fail with “Account not exist.”

**Now:** `AddressManager` persists and reloads known addresses using the same `Storage` backend as KV.

- Persist key: `state/known_accounts`
- Value format: newline-separated hex addresses (as returned by `CreateAccount`)
- Load happens when `AddressManager(Storage*)` is constructed.

Files changed:
- `executor/contract/manager/address_manager.{h,cpp}`
- `executor/contract/executor/contract_executor.cpp` (constructs `AddressManager(storage)`)

### B. Native transfer RPC: `TRANSFER_ROK`

**Now:** `proto/contract/rpc.proto` defines `TRANSFER_ROK = 6` plus fields:

- `from_account` (hex address)
- `to_account` (hex address)
- `amount` (hex uint256 string)

Server handling:
- Implemented in `ContractTransactionManager::TransferRoK`
- Validates both accounts exist (`AddressManager::Exist`)
- Reads balances via `ContractManager::GetBalance` (storage)
- Writes balances via `ContractManager::SetBalance` (storage)

Client support:
- `ContractClient::TransferRoK(from, to, amount_hex)`

Files changed:
- `proto/contract/rpc.proto`
- `executor/contract/executor/contract_executor.{h,cpp}`
- `interface/kv/contract_client.{h,cpp}`

### C. EVM balance consistency (load stored RoK balance on first touch)

**Problem before:** eEVM account state (`SimpleAccount.balance`) was created as 0 in `GlobalState::get()`, so contract execution could see zero balances even if `GETBALANCE/SETBALANCE` persisted something else.

**Now:** `GlobalState::get(addr)` loads persisted balance from storage (via `GetBalance(addr)`) and creates the in-memory `SimpleAccount` with that value on first access.

Files changed:
- `executor/contract/manager/global_state.cpp`

---

## 2) What must be tested (step-by-step)

### Test 0 — Build compatibility sanity

Because `rpc.proto` changed, server + client must be built from the same commit/branch for `TRANSFER_ROK` to work.

### Test 1 — Account persistence across restart (core)

1. Start KV service (unified path).
2. Create an account `A` (via ResContract CLI / ResVault / `ContractClient::CreateAccount`).
3. Restart the KV service (full process restart).
4. Using the same address `A`, attempt a contract operation that checks account existence:
   - `DEPLOY` or `EXECUTE` with `caller_address = A`
5. **Expected:** no “Account not exist.” due to restart (the account should be recognized after restart).

**If this fails:** inspect the `Storage` backend you’re using. Persistence requires a durable backend (LevelDB/DuckDB). If running `MemoryDB`, restart will always lose all state.

### Test 2 — Transfer RoK updates balances correctly

1. Create accounts `A` and `B`.
2. Give `A` an initial balance (temporary admin/dev step):
   - `SETBALANCE(A, <amount_hex>)`
3. Verify:
   - `GETBALANCE(A)` returns expected value
   - `GETBALANCE(B)` returns 0/empty (depending on API response)
4. Call:
   - `TRANSFER_ROK(from=A, to=B, amount=<x_hex>)`
5. Verify:
   - `GETBALANCE(A)` decreased by `x`
   - `GETBALANCE(B)` increased by `x`

Negative cases:
- Transfer with insufficient funds must fail
- Transfer where `from_account` doesn’t exist must fail
- Transfer where `to_account` doesn’t exist must fail
- Transfer with `amount=0` must fail

### Test 3 — Transfer state survives restart

1. Perform Test 2 (ensure balances have changed).
2. Restart KV service.
3. Re-run `GETBALANCE(A)` and `GETBALANCE(B)`.
4. **Expected:** balances remain as post-transfer values.

### Test 4 — EVM sees stored balances (consistency)

This depends on whether your deployed contracts/read paths actually consult eEVM-visible balance.

Minimum validation:
1. Set balance for `A` via `SETBALANCE`.
2. Execute a contract call (or deploy+execute) that causes eEVM to `get(A)` / touch `A`.
3. **Expected:** no unexpected “zero balance” behavior caused by in-memory init; first-touch should initialize from persisted storage.

---

## 3) Notes / limitations (known)

- `SETBALANCE` remains unrestricted “admin overwrite” in this change set. It should be gated/removed later if RoK is meant to be secure.
- Known accounts are stored in a **single key** (`state/known_accounts`) with newline-separated addresses (chosen for compatibility with current `Storage` backends and existing use of raw `SetValue`).

