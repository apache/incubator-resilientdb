package(default_visibility = ["//visibility:private"])

cc_library(
    name = "message_manager",
    srcs = ["message_manager.cpp"],
    hdrs = ["message_manager.h"],
    deps = [
        ":lock_free_collector_pool",
        "//platform/common/queue:lock_free_queue",
        "//platform/config:resdb_config",
        "//platform/consensus/execution:system_info",
        "//platform/consensus/execution:transaction_executor",
        #"//platform/consensus/ordering/common:data_collector",
        "//platform/consensus/ordering/poe/common:poe_utils",
        "//platform/consensus/ordering/poe/proto:poe_cc_proto",
        "//platform/networkstrate:replica_communicator",
        "//platform/networkstrate:server_comm",
        "//platform/proto:resdb_cc_proto",
    ],
)

cc_library(
    name = "lock_free_collector_pool",
    srcs = ["lock_free_collector_pool.cpp"],
    hdrs = ["lock_free_collector_pool.h"],
    deps = [
        ":transaction_collector",
    ],
)

cc_library(
    name = "transaction_collector",
    srcs = ["transaction_collector.cpp"],
    hdrs = ["transaction_collector.h"],
    deps = [
        "//platform/consensus/execution:transaction_executor",
        #"//platform/consensus/ordering/common:data_collector",
        "//platform/consensus/ordering/poe/common:transaction_state",
        "//platform/consensus/ordering/poe/proto:poe_cc_proto",
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
        "//platform/config:resdb_config",
        "//platform/consensus/ordering/poe/common:poe_utils",
        "//platform/consensus/ordering/poe/proto:poe_cc_proto",
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
        ":transaction_collector",
        "//platform/networkstrate:replica_communicator",
    ],
)

cc_library(
    name = "consensus",
    srcs = ["consensus.cpp"],
    hdrs = ["consensus.h"],
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
