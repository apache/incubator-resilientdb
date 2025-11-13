# RAFT integration roadmap

这是一份强制自己不瞎忙的工作清单，所有步骤按依赖顺序排好，完成后打钩。别跳步骤。

1. **配置和 proto 基础建设**
   - 更新 `platform/proto/replica_info.proto`：在 `ResConfigData` 里添加 `optional string consensus_protocol = 25;`。
   - 重新生成相关 proto 代码（`bazel build //...` 里的受影响 cc_proto targets）。
   - 扩展 `platform/config/resdb_config.h/.cpp`：新增 `GetConsensusProtocol()/SetConsensusProtocol()`，默认返回 `"pbft"` 以保持兼容。
   - 在现有配置样例里补上注释/占位，提醒可以设置成 `raft`。

2. **服务入口按配置切换共识实现**
   - 修改 `service/utils/server_factory.*`：保留默认 PBFT，同时提供 `CreateResDBServerForProtocol(const ResDBConfig&, ...)` 或等价逻辑，通过 `consensus_protocol` 决定实例化 `ConsensusManagerPBFT` 还是 `ConsensusManagerRaft`。
   - `service/kv/kv_service.cpp`：读取配置后判断 `config->GetConsensusProtocol()`，`raft` 时调用 `CustomGenerateResDBServer<ConsensusManagerRaft>`，否则走现状。
   - 同步其它服务入口（contract、benchmark 工具等），至少在 TODO 里标明需统一。
   - 建立服务入口 checklist，逐个勾掉并在代码评审里验证都通过 `consensus_protocol` 分支：
     - [x] `service/kv/kv_service.cpp`
     - [x] `service/contract/contract_service.cpp`
     - [x] `service/utxo/utxo_service.cpp`
     - [x] `ecosystem/graphql/service/kv_service/kv_server.cpp`
     - [ ] 其它使用 `GenerateResDBServer`/`CustomGenerateResDBServer` 的 binary（benchmark、tools、demo）。
   - 在 CI 里新增 PBFT/RAFT config matrix（最少覆盖 kv/contract/utxo/GraphQL KV server），保证两种协议都能启动并跑冒烟测试。
   - 定义协议切换失败路径：配置声明 `raft` 但依赖缺失时，`server_factory` 和各服务入口必须记录致命日志、回退到 PBFT 或直接终止，避免进入半初始化状态。

3. **RAFT proto 与消息类型常量**
   - 新建 `platform/consensus/ordering/raft/proto/raft.proto`，定义 `AppendEntries{Request,Response}`、`RequestVote{Request,Response}`、`InstallSnapshot{Request,Response}`、`SnapshotChunk` 等。
   - 配好 Bazel 目标（`raft_cc_proto`, `raft_proto`）。
   - 创建 `platform/consensus/ordering/raft/raft_message_type.h`，定义 `enum : int { RAFT_APPEND_ENTRIES = 1, ... }`，所有 `TYPE_CUSTOM_CONSENSUS` 消息都用这些常量。

4. **ConsensusManagerRaft 骨架**
   - 目录：`platform/consensus/ordering/raft/`.
   - 新建 `consensus_manager_raft.{h,cpp}`，继承 `ConsensusManager`，实现：
     - 构造函数注入 `TransactionManager`、`CustomQuery`。
     - 覆盖 `ConsensusCommit`（按 `Request::type()` 分流：客户端请求、RAFT RPC、自定义查询等）。
     - 覆盖 `GetPrimary`、`GetVersion`（映射为 leaderId / currentTerm）。
     - Heartbeat 使用 AppendEntries 空日志；必要时关掉基类 heartbeat 线程改用自定义调度。
   - 提供接口连接 RAFT 内核模块（log replication、state machine apply）。
   - [x] 基础骨架已就位（请求分流、主领导/term 追踪、心跳线程接口、可注入回调）。

5. **RAFT 内核组件**
   - **Key 前缀规范**：列出 PBFT 组件当前占用的 LevelDB key 空间（`checkpoint_manager`, `message_manager`, `recovery` 等），设计 `storage::PrefixAllocator` 或等价机制，确保 `raft_state_*`/`raft_log_*`/`raft_snapshot_*` 与旧前缀彻底隔离，并用单元测试覆盖冲突检测。
   - **日志结构**：实现 `RaftLog` 类，负责内存+持久层（复用 storage 接口，使用前缀区分：`raft_state_`, `raft_log_`, `raft_snapshot_`）。
     - 支持 `Append`, `Truncate`, `CommitTo`, `LastLogIndex/Term`。
     - 快速加载 LevelDB 前缀以恢复。
   - **持久化状态**：`RaftPersistentState` 记录 `currentTerm`, `votedFor`, `lastApplied`, `snapshot` 元信息。
   - **快照管理**：`RaftSnapshotManager`，负责生成快照（从 `TransactionExecutor`/业务存储拉数据）并通过 `InstallSnapshot` 传播；落盘时与 PBFT 的 `checkpoint_manager` 保持互不干扰。
   - **网络 RPC**：实现 `RaftRpc`，把 `AppendEntries/RequestVote/InstallSnapshot` 通过 `ReplicaCommunicator` 用 `TYPE_CUSTOM_CONSENSUS + user_type` 发送。

6. **节点角色与状态机**
   - `RaftNode`（或等价类）管理角色转换（Follower/Leader/Candidate）：
     - 计时器（选举、心跳），用 `absl::Time` 或 `std::chrono`。
     - 投票逻辑、日志一致性检查、commit index 推进。
     - 和 `TransactionExecutor` 协作：leader 将客户端请求写成日志条目，达到 quorum 后调用 `TransactionExecutor::Commit`。

7. **恢复/快照集成**
   - 扩展 `platform/consensus/recovery/recovery.cpp`：`TYPE_CUSTOM_CONSENSUS` 分支根据 `user_type` 解析 RAFT 日志/快照。
   - 为 RAFT 日志追加流程添加持久化钩子，确保崩溃恢复时能重放。
   - 和现有查询接口 (`platform/consensus/ordering/pbft/query.*`) 对齐，提供获取 commit index、日志条目、快照的 API。

8. **配置与运维工具同步**
   - 更新 `documents/`、`scripts/deploy` 下的样例配置/脚本，描述如何选择 `consensus_protocol = "raft"`、最佳参数（选举超时、心跳间隔等）。
   - 在 README/CHANGELOG 里声明新增 RAFT 选项、兼容性注意事项（BFT→CFT 的语义差异）。
   - 列出所有对外接口（proto、CLI、脚本、监控 API），逐项确认无需版本协商或能通过配置字段保持向后兼容。

9. **测试与验证**
   - 单元测试：`RaftLog`、`RaftPersistentState`、RPC 编/解码、状态转换。
   - 集成测试：最少 3 节点 RAFT 集群跑 KV，验证选举、日志复制、快照、恢复。
   - 性能比较实验（可后续）：PBFT vs RAFT。
   - PBFT 回归：沿用现有 PBFT 测试/benchmark，在 `consensus_protocol = "pbft"` 配置下跑一遍 CI matrix，确认行为、性能和 API 兼容性不被 RAFT 改动污染。

10. **后续清理**
    - 统一 server 工程里所有 `GenerateResDBServer` 调用的协议选择逻辑。
    - 观察 `Stats`、监控（`ecosystem/monitoring`）可视化是否需要展示 RAFT 指标（term、leader、commit index）。
    - 评估是否要把 PBFT 旧代码抽象化，减少重复（未来工作）。

执行顺序不可乱：先搞定配置 & proto，再写骨架，再逐步填内核和恢复。每一步完成记得在本文件更新状态。

---

## Pending clarifications

1. **性能影响**
   - RAFT 模块本身不应拖慢 PBFT，但要评估共享资源（`Stats`、`ReplicaCommunicator` 线程池、LevelDB IO）是否会被新逻辑污染。
   - TODO：计划一套性能基准（PBFT baseline、RAID 各场景）并记录指标；如有必要拆分线程池或提供配置隔离。
