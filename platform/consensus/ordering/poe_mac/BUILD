package(default_visibility = ["//visibility:private"])

cc_library(
    name = "poe_utils",
    srcs = ["poe_utils.cpp"],
    hdrs = ["poe_utils.h"],
    deps = [
        "//platform/consensus/ordering/poe_mac/proto:poe_cc_proto",
        "//platform/proto:resdb_cc_proto",
    ],
)

cc_library(
    name = "message_manager",
    srcs = ["message_manager.cpp"],
    hdrs = ["message_manager.h"],
    deps = [
        ":poe_lock_free_collector_pool",
        ":poe_utils",
        "//platform/common/queue:lock_free_queue",
        "//platform/config:resdb_config",
        "//platform/consensus/execution:system_info",
        "//platform/consensus/execution:transaction_executor",
        "//platform/consensus/ordering/common:data_collector",
        "//platform/consensus/ordering/poe_mac/proto:poe_cc_proto",
        "//platform/networkstrate:replica_communicator",
        "//platform/networkstrate:server_comm",
        "//platform/proto:resdb_cc_proto",
    ],
)

cc_library(
    name = "poe_lock_free_collector_pool",
    srcs = ["poe_lock_free_collector_pool.cpp"],
    hdrs = ["poe_lock_free_collector_pool.h"],
    deps = [
        ":poe_transaction_collector",
    ],
)

cc_library(
    name = "poe_transaction_collector",
    srcs = ["poe_transaction_collector.cpp"],
    hdrs = ["poe_transaction_collector.h"],
    deps = [
        "//platform/consensus/execution:transaction_executor",
        "//platform/consensus/ordering/common:data_collector",
        "//platform/consensus/ordering/poe_mac/proto:poe_cc_proto",
        "//platform/networkstrate:server_comm",
        "//platform/statistic:stats",
    ],
)

cc_library(
    name = "viewchange",
    srcs = ["viewchange.cpp"],
    hdrs = ["viewchange.h"],
    deps = [
        ":commitment",
        ":message_manager",
        ":poe_utils",
        "//platform/config:resdb_config",
        "//platform/consensus/ordering/poe_mac/proto:poe_cc_proto",
        "//platform/networkstrate:replica_communicator",
        "//platform/proto:resdb_cc_proto",
    ],
)

cc_library(
    name = "commitment",
    srcs = ["commitment.cpp"],
    hdrs = ["commitment.h"],
    deps = [
        ":message_manager",
        "//platform/networkstrate:replica_communicator",
    ],
)

cc_library(
    name = "consensus_manager_poe",
    srcs = ["consensus_manager_poe.cpp"],
    hdrs = ["consensus_manager_poe.h"],
    visibility = ["//visibility:public"],
    deps = [
        ":commitment",
        ":message_manager",
        ":viewchange",
        "//common/crypto:signature_verifier",
        "//platform/consensus/ordering/common:performance_manager",
        "//platform/consensus/ordering/common:response_manager",
        "//platform/networkstrate:consensus_manager",
    ],
)
