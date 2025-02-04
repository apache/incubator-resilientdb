package(default_visibility = ["//visibility:private"])

cc_library(
    name = "consensus",
    srcs = ["consensus.cpp"],
    hdrs = ["consensus.h"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//common/utils",
        "//executor/common:transaction_manager",
        "//platform/consensus/execution:transaction_executor",
        "//platform/consensus/ordering/common/framework:consensus",
        "//platform/consensus/ordering/tusk/protocol:tusk",
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
