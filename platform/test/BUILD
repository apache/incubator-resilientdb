package(default_visibility = ["//visibility:private"])

cc_test(
    name = "resdb_test",
    srcs = ["resdb_test.cpp"],
    data = [
        "//platform/test/test_data",
    ],
    deps = [
        "//common:comm",
        "//common/test:test_main",
        "//executor/common:transaction_manager",
        "//interface/rdbc:transaction_constructor",
        "//platform/config:resdb_config_utils",
        "//platform/consensus/ordering/pbft:consensus_manager_pbft",
        "//platform/networkstrate:service_network",
        "//platform/test/proto:resdb_test_cc_proto",
    ],
)
