package(default_visibility = ["//visibility:private"])

cc_library(
    name = "consensus_manager_geo_pbft",
    srcs = ["consensus_manager_geo_pbft.cpp"],
    hdrs = ["consensus_manager_geo_pbft.h"],
    visibility = ["//visibility:public"],
    deps = [
        ":geo_pbft_commitment",
        "//platform/consensus/execution:geo_transaction_executor",
        "//platform/consensus/ordering/pbft:consensus_manager_pbft",
    ],
)

cc_library(
    name = "geo_pbft_commitment",
    srcs = ["geo_pbft_commitment.cpp"],
    hdrs = ["geo_pbft_commitment.h"],
    deps = [
        ":hash_set",
        "//common:comm",
        "//platform/communication:replica_communicator",
        "//platform/communication:server_comm",
        "//platform/config:resdb_config",
        "//platform/consensus/execution:geo_global_executor",
        "//platform/consensus/execution:system_info",
        "//platform/consensus/ordering/common:transaction_utils",
        "//platform/proto:resdb_cc_proto",
    ],
)

cc_library(
    name = "hash_set",
    hdrs = ["hash_set.h"],
)

cc_test(
    name = "geo_pbft_commitment_test",
    srcs = ["geo_pbft_commitment_test.cpp"],
    tags = ["manual"],
    deps = [
        ":geo_pbft_commitment",
        "//common/crypto:mock_signature_verifier",
        "//common/test:test_main",
        "//platform/communication:mock_replica_communicator",
        "//platform/config:resdb_config_utils",
        "//platform/consensus/execution:mock_geo_global_executor",
        "//platform/consensus/execution:mock_transaction_executor_impl",
    ],
)
