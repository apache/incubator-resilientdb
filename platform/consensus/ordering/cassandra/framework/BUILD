package(default_visibility = ["//visibility:private"])

cc_library(
    name = "response_manager",
    srcs = ["response_manager.cpp"],
    hdrs = ["response_manager.h"],
    deps = [
        "//platform/consensus/ordering/framework:transaction_utils",
        "//platform/networkstrate:replica_communicator",
        "//platform/networkstrate:server_comm",
    ],
)

cc_library(
    name = "performance_manager",
    srcs = ["performance_manager.cpp"],
    hdrs = ["performance_manager.h"],
    deps = [
        "//platform/consensus/ordering/framework:transaction_utils",
        "//platform/networkstrate:replica_communicator",
        "//platform/networkstrate:server_comm",
    ],
)

cc_library(
    name = "consensus",
    srcs = ["consensus.cpp"],
    hdrs = ["consensus.h"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":performance_manager",
        ":response_manager",
        "//common/utils",
        "//executor/common:transaction_manager",
        "//platform/consensus/execution:transaction_executor",
        "//platform/consensus/ordering/cassandra/algorithm/basic:cassandra",
        "//platform/consensus/ordering/cassandra/algorithm/fast_path:cassandra",
        "//platform/consensus/ordering/cassandra/algorithm/mem:cassandra",
        "//platform/consensus/ordering/cassandra/algorithm/mem_recovery:cassandra",
        "//platform/networkstrate:consensus_manager",
    ],
)

cc_test(
    name = "consensus_test",
    srcs = ["consensus_test.cpp"],
    deps = [
        ":consensus",
        "//common/test:test_main",
        "//executor/common:mock_transaction_manager",
        "//platform/config:resdb_config_utils",
        "//platform/networkstrate:mock_replica_communicator",
    ],
)
