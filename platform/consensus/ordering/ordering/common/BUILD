package(default_visibility = ["//platform:__subpackages__"])

cc_library(
    name = "data_collector",
    srcs = ["data_collector.cpp"],
    hdrs = ["data_collector.h"],
    deps = [
        "//platform/consensus/execution:transaction_executor",
        "//platform/proto:resdb_cc_proto",
    ],
)

cc_library(
    name = "data_collector_pool",
    srcs = ["data_collector_pool.cpp"],
    hdrs = ["data_collector_pool.h"],
    deps = [
        ":data_collector",
    ],
)

cc_library(
    name = "performance_generator",
    srcs = ["performance_generator.cpp"],
    hdrs = ["performance_generator.h"],
    deps = [
        ":request_handler",
        "//platform/consensus/ordering/common:data_collector_pool",
        "//platform/networkstrate:replica_communicator",
        "//platform/networkstrate:server_comm",
    ],
)

cc_library(
    name = "commitment_basic",
    srcs = ["commitment_basic.cpp"],
    hdrs = ["commitment_basic.h"],
    deps = [
        ":message_manager_basic",
        "//platform/networkstrate:replica_communicator",
        "//platform/networkstrate:server_comm",
    ],
)

cc_library(
    name = "message_manager_basic",
    srcs = ["message_manager_basic.cpp"],
    hdrs = ["message_manager_basic.h"],
    deps = [
        "//executor/common:transaction_manager",
        "//platform/common/queue:lock_free_queue",
        "//platform/config:resdb_config",
        "//platform/consensus/execution:system_info",
        "//platform/consensus/execution:transaction_executor",
        "//platform/networkstrate:server_comm",
        "//platform/proto:resdb_cc_proto",
    ],
)

cc_library(
    name = "transaction_utils",
    srcs = ["transaction_utils.cpp"],
    hdrs = ["transaction_utils.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//platform/proto:resdb_cc_proto",
    ],
)

cc_library(
    name = "request_handler",
    hdrs = ["request_handler.h"],
    deps = [
    ],
)

cc_library(
    name = "response_manager",
    srcs = ["response_manager.cpp"],
    hdrs = ["response_manager.h"],
    deps = [
        ":request_handler",
        "//platform/networkstrate:replica_communicator",
        "//platform/networkstrate:server_comm",
    ],
)

cc_library(
    name = "performance_manager",
    srcs = ["performance_manager.cpp"],
    hdrs = ["performance_manager.h"],
    deps = [
        ":request_handler",
        "//platform/consensus/ordering/common:data_collector_pool",
        "//platform/networkstrate:replica_communicator",
        "//platform/networkstrate:server_comm",
    ],
)
