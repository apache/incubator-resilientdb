package(default_visibility = ["//visibility:public"])

cc_library(
    name = "shard_metadata",
    srcs = ["shard_metadata.cpp"],
    hdrs = ["shard_metadata.h"],
    deps = [
        "//common:json",
    ],
)

cc_library(
    name = "shard_communicator",
    srcs = ["shard_communicator.cpp"],
    hdrs = ["shard_communicator.h"],
    deps = [
        ":shard_metadata",
        "//platform/networkstrate:replica_communicator",
        "//platform/proto:replica_info_cc_proto",
    ],
)

cc_library(
    name = "consensus_manager_sharded_3pc",
    srcs = ["consensus_manager_sharded_3pc.cpp"],
    hdrs = ["consensus_manager_sharded_3pc.h"],
    deps = [
        ":shard_metadata",
        "//common:comm",
        "//executor/common:transaction_manager",
        "//platform/config:resdb_config",
        "//platform/consensus/ordering/pbft:consensus_manager_pbft",
    ],
)

cc_test(
    name = "shard_metadata_test",
    srcs = ["shard_metadata_test.cpp"],
    deps = [
        ":shard_metadata",
        "//common/test:test_main",
    ],
)

cc_test(
    name = "shard_communicator_test",
    srcs = ["shard_communicator_test.cpp"],
    deps = [
        ":shard_communicator",
        ":shard_metadata",
        "//common/test:test_main",
        "//platform/networkstrate:replica_communicator",
        "//platform/proto:replica_info_cc_proto",
        "//platform/proto:resdb_cc_proto",
    ],
)
