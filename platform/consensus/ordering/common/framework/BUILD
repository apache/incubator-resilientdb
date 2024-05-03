package(default_visibility = ["//platform/consensus/ordering:__subpackages__"])

cc_library(
    name = "consensus",
    srcs = ["consensus.cpp"],
    hdrs = ["consensus.h"],
    deps = [
        ":performance_manager",
        ":response_manager",
        "//common/utils",
        "//executor/common:transaction_manager",
        "//platform/consensus/execution:transaction_executor",
        "//platform/consensus/ordering/common/algorithm:protocol_base",
        "//platform/networkstrate:consensus_manager",
    ],
)

cc_library(
    name = "performance_manager",
    srcs = ["performance_manager.cpp"],
    hdrs = ["performance_manager.h"],
    deps = [
        ":transaction_utils",
        "//platform/networkstrate:replica_communicator",
        "//platform/networkstrate:server_comm",
    ],
)

cc_library(
    name = "response_manager",
    srcs = ["response_manager.cpp"],
    hdrs = ["response_manager.h"],
    deps = [
        ":transaction_utils",
        "//platform/networkstrate:replica_communicator",
        "//platform/networkstrate:server_comm",
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
