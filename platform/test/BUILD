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
        "//platform/communication:service_network",
        "//platform/config:resdb_config_utils",
        "//platform/consensus/execution:transaction_manager",
        "//platform/consensus/ordering/pbft:consensus_manager_pbft",
        "//platform/interface:resdb_transaction_proposer",
        "//platform/test/proto:resdb_test_cc_proto",
    ],
)
