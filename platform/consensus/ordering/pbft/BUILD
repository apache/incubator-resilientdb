package(default_visibility = ["//visibility:private"])

cc_library(
    name = "lock_free_collector_pool",
    srcs = ["lock_free_collector_pool.cpp"],
    hdrs = ["lock_free_collector_pool.h"],
    deps = [
        ":transaction_collector",
    ],
)

cc_test(
    name = "lock_free_collector_pool_test",
    srcs = ["lock_free_collector_pool_test.cpp"],
    deps = [
        ":lock_free_collector_pool",
        "//common/test:test_main",
    ],
)

cc_library(
    name = "response_manager",
    srcs = ["response_manager.cpp"],
    hdrs = ["response_manager.h"],
    deps = [
        ":lock_free_collector_pool",
        ":transaction_utils",
        "//platform/communication:replica_communicator",
    ],
)

cc_test(
    name = "response_manager_test",
    srcs = ["response_manager_test.cpp"],
    deps = [
        ":response_manager",
        "//common/test:test_main",
        "//platform/communication:mock_replica_communicator",
        "//platform/config:resdb_config_utils",
        "//platform/interface:mock_resdb_net_channel",
    ],
)

cc_library(
    name = "performance_manager",
    srcs = ["performance_manager.cpp"],
    hdrs = ["performance_manager.h"],
    deps = [
        ":lock_free_collector_pool",
        ":transaction_utils",
        "//platform/communication:replica_communicator",
    ],
)

cc_library(
    name = "message_manager",
    srcs = ["message_manager.cpp"],
    hdrs = ["message_manager.h"],
    deps = [
        ":checkpoint_manager",
        ":lock_free_collector_pool",
        ":transaction_collector",
        ":transaction_utils",
        "//platform/communication:server_comm",
        "//platform/config:resdb_config",
        "//platform/consensus/execution:transaction_manager",
        "//platform/proto:resdb_cc_proto",
        "//storage:txn_memory_db",
    ],
)

cc_library(
    name = "transaction_utils",
    srcs = ["transaction_utils.cpp"],
    hdrs = ["transaction_utils.h"],
    deps = [
        "//platform/proto:resdb_cc_proto",
    ],
)

cc_library(
    name = "commitment",
    srcs = ["commitment.cpp"],
    hdrs = ["commitment.h"],
    deps = [
        ":message_manager",
        ":response_manager",
        "//common/utils",
        "//platform/common/queue:batch_queue",
        "//platform/communication:replica_communicator",
        "//platform/config:resdb_config",
        "//platform/proto:resdb_cc_proto",
        "//platform/statistic:stats",
    ],
)

cc_test(
    name = "commitment_test",
    srcs = ["commitment_test.cpp"],
    deps = [
        ":commitment",
        "//common/crypto:mock_signature_verifier",
        "//common/test:test_main",
        "//platform/communication:mock_replica_communicator",
        "//platform/config:resdb_config_utils",
        "//platform/interface:mock_resdb_net_channel",
    ],
)

cc_library(
    name = "checkpoint_manager",
    srcs = ["checkpoint_manager.cpp"],
    hdrs = ["checkpoint_manager.h"],
    deps = [
        ":transaction_utils",
        "//common/crypto:signature_verifier",
        "//platform/communication:replica_communicator",
        "//platform/communication:server_comm",
        "//platform/config:resdb_config",
        "//platform/proto:checkpoint_info_cc_proto",
        "//storage:txn_memory_db",
    ],
)

cc_test(
    name = "checkpoint_manager_test",
    srcs = ["checkpoint_manager_test.cpp"],
    deps = [
        ":checkpoint_manager",
        "//common/crypto:mock_signature_verifier",
        "//common/test:test_main",
        "//platform/communication:mock_replica_communicator",
        "//platform/config:resdb_config_utils",
        "//platform/statistic:stats",
    ],
)

cc_library(
    name = "viewchange_manager",
    srcs = ["viewchange_manager.cpp"],
    hdrs = ["viewchange_manager.h"],
    deps = [
        ":checkpoint_manager",
        ":message_manager",
        ":transaction_utils",
        "//platform/communication:replica_communicator",
        "//platform/config:resdb_config",
        "//platform/consensus/execution:system_info",
        "//platform/proto:viewchange_message_cc_proto",
    ],
)

cc_test(
    name = "viewchange_manager_test",
    srcs = ["viewchange_manager_test.cpp"],
    deps = [
        ":viewchange_manager",
        "//common/crypto:mock_signature_verifier",
        "//common/test:test_main",
        "//platform/communication:mock_replica_communicator",
        "//platform/config:resdb_config_utils",
        "//platform/consensus/execution:system_info",
    ],
)

cc_library(
    name = "query",
    srcs = ["query.cpp"],
    hdrs = ["query.h"],
    deps = [
        ":message_manager",
        "//platform/config:resdb_config",
        "//platform/consensus/execution:custom_query",
        "//platform/proto:resdb_cc_proto",
    ],
)

cc_test(
    name = "query_test",
    srcs = ["query_test.cpp"],
    deps = [
        ":commitment",
        ":query",
        "//common/crypto:mock_signature_verifier",
        "//common/test:test_main",
        "//platform/communication:mock_replica_communicator",
        "//platform/config:resdb_config_utils",
        "//platform/interface:mock_resdb_net_channel",
    ],
)

cc_library(
    name = "transaction_collector",
    srcs = ["transaction_collector.cpp"],
    hdrs = ["transaction_collector.h"],
    deps = [
        "//platform/communication:server_comm",
        "//platform/consensus/execution:transaction_executor",
        "//platform/proto:resdb_cc_proto",
        "//platform/statistic:stats",
    ],
)

cc_test(
    name = "transaction_collector_test",
    srcs = ["transaction_collector_test.cpp"],
    deps = [
        ":transaction_collector",
        "//common/test:test_main",
    ],
)

cc_library(
    name = "consensus_manager_pbft",
    srcs = ["consensus_manager_pbft.cpp"],
    hdrs = ["consensus_manager_pbft.h"],
    visibility = [
        "//benchmark:__subpackages__",
        "//platform:__subpackages__",
        "//service:__subpackages__",
    ],
    deps = [
        ":checkpoint_manager",
        ":commitment",
        ":message_manager",
        ":performance_manager",
        ":query",
        ":viewchange_manager",
        "//common/crypto:signature_verifier",
        "//platform/communication:consensus_manager",
    ],
)

cc_library(
    name = "pre_very_consensus_manager_pbft",
    hdrs = ["pre_very_consensus_manager_pbft.h"],
    visibility = [
        "//platform:__subpackages__",
        "//service:__subpackages__",
    ],
    deps = [
        ":consensus_manager_pbft",
    ],
)
