# Sync Fides

Fides 的同步网络变体 —— 在同步假设下把 wave 折叠到 k=1，去掉 T-Coin，保留 MC + RAC + T-RBC，达成 fides 同档吞吐 + 更低 commit latency。

## 1. 为什么有 Sync Fides

原 Fides 是 **异步** DAG-BFT，用 4-round wave + T-Coin 随机 leader 对抗异步对手。在同步网络下：

- 异步对手的"先知 leader → 抑制消息"攻击天然失效（消息延迟有 Δ 上界）
- T-Coin / DKG 不再必要 → 用 round-robin 确定性 leader
- 4-round wave 可以折叠到 1-round（每轮一个 leader，每轮可 commit）
- TEE 表面缩小：只剩 MC + RAC，不需要 RNG

设计目标：

```
n = 2f+1          (与原 Fides 一致)
k = 1             (每轮一个 leader)
leader = (r % n) + 1   (round-robin)
commit = 2Δ            (理论下界)
```

完整设计文档：[`docs/superpowers/specs/2026-04-07-sync-fides-design.md`](../../../../docs/superpowers/specs/2026-04-07-sync-fides-design.md)

## 2. 协议结构

### 2.1 Dissemination Layer（与 Fides 共享）

| 组件 | 作用 | 是否复用 Fides |
|---|---|---|
| **MC** (Monotonic Counter) | TEE 内单调计数器，防 vertex 等价 | ✅ 完全复用 |
| **RAC** (Round Advancement Certifier) | 验证 f+1 references 的紧凑证书 | ✅ 完全复用 |
| **T-RBC** (TEE Reliable Broadcast) | 单跳广播 vertex | ✅ 完全复用 |
| **T-Coin / RNG / DKG** | 随机 leader 选举 | ❌ **删除** |

### 2.2 Ordering Layer（Sync Fides 的核心改动）

**Leader 选举**: `L(r) = (r % n) + 1` —— 完全确定性。

**Round 推进**: 收到 f+1 round-r 的 cert 即可推进到 round r+1。**不等 leader cert**（保 throughput）。

**Direct commit rule**:
> Leader L 在 round r 被 commit ⟺ 至少 f+1 个 vertex 直接引用 L 的 round-r vertex（1-hop）

引用计数同时累积 **strong_cert + weak_cert**（关键 trick，下面解释）。

**Skipped leader**: 如果 leader 一时没集齐 f+1 引用，由后续 leader 的 recursive commit 兜底拉入。

## 3. 三个关键 trick

### Trick 1: 旋转 strong_cert 的选择起点

**问题**: `cert_list_` 是 `std::map<sender_id, ...>`，原 Fides 总是取前 f+1 个 sender → 永远只引用 nodes {1..f+1}，round-robin leader 落到 {f+2..n} 时永远被跳过。

**修复** (`proposal_manager.cpp:GetMetaData`):
```cpp
int prev_leader = ((round_ - 1) % total_num) + 1;
for (int offset = 0; offset < total_num; ++offset) {
  int sender = ((prev_leader - 1 + offset) % total_num) + 1;
  // ... 把 cert_list_ 中存在的 sender 加入 strong_cert
  if (count == limit_count_) break;
}
```

**效果**: 优先把上一轮 leader 放进 strong_cert，剩余 sender 按旋转顺序填充。

### Trick 2: weak_cert 也参与 reference 计数

**问题**: 即使有 rotation，由于网络抖动，leader cert 有时没赶上下一轮的 strong_cert quorum。strong_cert 错过后这个 leader 就再也不会被引用 → direct rule 永不触发。

**修复** (`proposal_manager.cpp:AddCert`):
```cpp
for (auto& link : cert->strong_cert().cert()) {
  reference_[(link.round, link.proposer)].push_back(cert->proposer());
}
// NEW: weak_cert 也算
for (auto& link : cert->weak_cert().cert()) {
  reference_[(link.round, link.proposer)].push_back(cert->proposer());
}
```

同时改 proto: `Certificate` 加 `CertLink weak_cert = 7;`。`sync_fides.cpp` 创建 cert 时也复制 `proposal->header().weak_cert()`。

**效果**: 即使 leader 没进 strong_cert，未来若干轮的 vertex 会通过 weak_cert 引用它（因为 `latest_cert_from_sender_[L]` 一定包含 L 的最近 cert）。引用累积到 ≥ f+1 后，direct rule 触发。

### Trick 3: 1-hop direct reference count

**问题**: 原 Fides 用 2-hop `GetReferenceNum`（leader → r+1 → r+2 嵌套查找），这个语义假设第 1 跳的 referrer 出现在 round (r+1)。但 weak_cert ref 来自更晚的 round（比如 r+5），第 2 跳查找会失败。

**修复** (`proposal_manager.cpp:GetDirectReferenceNum`):
```cpp
int GetDirectReferenceNum(int round, int proposer) {
  return distinct_set(reference_[(round, proposer)]).size();
}
```

`AsyncCommit` 用这个新方法：
```cpp
int reference_num = proposal_manager_->GetDirectReferenceNum(r, leader);
if (reference_num >= limit_count_) CommitProposal(r, leader);
```

**安全性**: 1-hop 在 sync + MC 假设下是安全的：
- MC 已经防止了 leader vertex 等价性
- f+1 个直接引用足以保证所有诚实 replica commit 同一个 leader vertex

## 4. 性能（8 节点 LAN, KV workload, 50B value）

| 协议 | max tps | avg tps | commit_round avg | per-tx latency avg |
|---|---|---|---|---|
| Fides (k=4) | 348k | 322k | 3.39 | 0.028s |
| **Sync Fides (k=1)** | **369k** | **333k** | **4.90** | **0.024s** |

- ✅ throughput **超过 fides** (avg 103%, max 106%)
- ✅ per-tx latency **优于 fides** (-14%)
- ✅ commit_round latency 比 fides 略高（4.9 vs 3.4），但已经压住了 tail（max 7.5 vs 之前的 47）

## 5. 代码 layout

```
platform/consensus/ordering/sync_fides/
├── algorithm/
│   ├── sync_fides.{h,cpp}      # 主协议（GetLeader, AsyncCommit, AsyncSend）
│   ├── proposal_manager.{h,cpp}# DAG state（GetMetaData, AddCert, Ready）
│   ├── utility.{h,cpp}         # 共享工具
│   └── BUILD
├── framework/
│   ├── consensus.{h,cpp}       # ResilientDB consensus 框架接入
│   └── BUILD
└── proto/
    ├── proposal.proto          # 消息格式（Certificate 含 weak_cert）
    └── BUILD

benchmark/protocols/sync_fides/
├── kv_server_performance.cpp   # KV 性能测试入口
└── BUILD

scripts/deploy/
├── performance/sync_fides_performance.sh
└── config/sync_fides.config    # rng_enabled=0
```

## 6. 编译与运行

```bash
# 编译
bazel build //benchmark/protocols/sync_fides:kv_server_performance

# 运行性能测试（需要 8 个 replica 容器在 172.17.0.6-13:2222）
cd scripts/deploy
./performance/sync_fides_performance.sh ./config/kv_server.conf
```

## 7. 关键改动 vs Fides

| 文件 | 函数 | 改动 |
|---|---|---|
| `sync_fides.cpp` | `GetLeader` | T-Coin → `(r % n) + 1` |
| `sync_fides.cpp` | `TEECommonCoin` | stub `return 0` |
| `sync_fides.cpp` | `AsyncCommit` | k=4 stride → k=1，用 `GetDirectReferenceNum`，wait_attempts 50→5 |
| `sync_fides.cpp` | `AsyncSend` | `CommitRound(round - 3)` → `CommitRound(round - 2)` |
| `sync_fides.cpp` | `ReceiveBlock` / cert 创建处 (3 处) | 复制 `weak_cert` 到 cert |
| `proposal_manager.cpp` | `GetMetaData` | 旋转选 sender，prev_leader 优先 |
| `proposal_manager.cpp` | `AddCert` | 同时把 `weak_cert` links 写进 `reference_` |
| `proposal_manager.cpp` | `GetDirectReferenceNum` | **新增**：1-hop ref count |
| `proto/proposal.proto` | `Certificate` | 加 `CertLink weak_cert = 7;` |

## 8. 已知 v1 限制 / v2 待办

- ❌ **没有 Δ_round / Δ_idle / Δ_commit timer** —— 当前 Byzantine leader 的 fault 检测靠隐式 `wait_attempts × 100μs`，不够鲁棒。spec §6.1 设计了显式 timer
- ❌ **没有 heartbeat vertex** —— mempool 空时 DAG 会停。spec §6.2
- ❌ **没有显式 SkippedLeaders set** —— 当前用 `previous_round` 隐式追踪
- ❌ **没有形式化安全证明** —— spec §7.5 只有 sketch，需要补完整证明

## 9. 设计取舍记录

| 尝试过的方案 | 结果 | 为什么不要 |
|---|---|---|
| `CommitRound(round)` 立即触发 | commit_round 800+ rounds | 2-hop check 时 round (r+2) 还不存在 |
| `Ready()` 阻塞等 prev_leader cert | commit_round 1.81，throughput 掉 33% | 同步串行化，每轮多 Δ |
| `Ready()` 等 + 2ms timeout | commit_round 21（更糟） | DAG 分叉，未等的 replica 漏 leader |
| `Ready()` 等 + warmup 跳 50/1000 轮 | warmup 不是瓶颈 | wait 是每轮常数代价，不是 warmup 问题 |
| 1-hop 但 strong_cert only | commit_round 18 | 没 weak_cert 兜底，leader 漏率高 |
| **1-hop + weak_cert + rotation** | **throughput 持平 + commit_round 4.9** | ✅ 当前方案 |
