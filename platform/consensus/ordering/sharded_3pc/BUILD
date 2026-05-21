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
    name = "shard_router",
    srcs = ["shard_router.cpp"],
    hdrs = ["shard_router.h"],
    deps = [
        ":shard_metadata",
    ],
)

cc_library(
    name = "sharded_response_manager",
    srcs = ["sharded_response_manager.cpp"],
    hdrs = ["sharded_response_manager.h"],
    deps = [
        ":shard_metadata",
        ":shard_router",
        "//platform/consensus/ordering/pbft:response_manager",
    ],
)

cc_library(
    name = "shard_pbft_commitment",
    srcs = ["shard_pbft_commitment.cpp"],
    hdrs = ["shard_pbft_commitment.h"],
    deps = [
        ":shard_communicator",
        "//platform/consensus/ordering/pbft:commitment",
    ],
)

cc_library(
    name = "shard_3pc_commitment",
    srcs = ["shard_3pc_commitment.cpp"],
    hdrs = ["shard_3pc_commitment.h"],
    deps = [
        ":shard_communicator",
        ":shard_metadata",
        "//platform/consensus/ordering/3pc:commitment_3pc",
    ],
)

cc_library(
    name = "shard_poe_commitment",
    srcs = ["shard_poe_commitment.cpp"],
    hdrs = ["shard_poe_commitment.h"],
    deps = [
        ":shard_communicator",
        ":shard_metadata",
        "//platform/consensus/ordering/pbft:commitment",
    ],
)

cc_library(
    name = "consensus_manager_sharded_3pc",
    srcs = ["consensus_manager_sharded_3pc.cpp"],
    hdrs = ["consensus_manager_sharded_3pc.h"],
    deps = [
        ":shard_3pc_commitment",
        ":shard_communicator",
        ":shard_metadata",
        ":shard_pbft_commitment",
        ":sharded_response_manager",
        "//common:comm",
        "//executor/common:transaction_manager",
        "//platform/config:resdb_config",
        "//platform/consensus/ordering/3pc:commitment_3pc",
        "//platform/consensus/ordering/pbft:consensus_manager_pbft",
    ],
)

cc_library(
    name = "consensus_manager_sharded_3pc_poe",
    srcs = ["consensus_manager_sharded_3pc_poe.cpp"],
    hdrs = ["consensus_manager_sharded_3pc_poe.h"],
    deps = [
        ":shard_3pc_commitment",
        ":shard_communicator",
        ":shard_metadata",
        ":shard_poe_commitment",
        ":sharded_response_manager",
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

cc_test(
    name = "shard_router_test",
    srcs = ["shard_router_test.cpp"],
    deps = [
        ":shard_metadata",
        ":shard_router",
        "//common/test:test_main",
    ],
)

cc_test(
    name = "sharded_response_manager_test",
    srcs = ["sharded_response_manager_test.cpp"],
    deps = [
        ":shard_metadata",
        ":sharded_response_manager",
        "//common/test:test_main",
        "//platform/config:resdb_config_utils",
        "//platform/consensus/execution:system_info",
        "//platform/networkstrate:replica_communicator",
        "//platform/proto:resdb_cc_proto",
    ],
)

cc_test(
    name = "sharded_commitment_test",
    srcs = ["sharded_commitment_test.cpp"],
    deps = [
        ":shard_3pc_commitment",
        ":shard_communicator",
        ":shard_metadata",
        ":shard_pbft_commitment",
        "//common/crypto:mock_signature_verifier",
        "//common/test:test_main",
        "//platform/config:resdb_config_utils",
        "//platform/consensus/execution:system_info",
        "//platform/consensus/ordering/pbft:checkpoint_manager",
        "//platform/consensus/ordering/pbft:message_manager",
        "//platform/networkstrate:replica_communicator",
        "//platform/proto:resdb_cc_proto",
    ],
)

cc_test(
    name = "shard_poe_commitment_test",
    srcs = ["shard_poe_commitment_test.cpp"],
    deps = [
        ":shard_communicator",
        ":shard_metadata",
        ":shard_poe_commitment",
        "//common/crypto:mock_signature_verifier",
        "//common/test:test_main",
        "//platform/config:resdb_config_utils",
        "//platform/consensus/execution:system_info",
        "//platform/consensus/ordering/pbft:checkpoint_manager",
        "//platform/consensus/ordering/pbft:message_manager",
        "//platform/networkstrate:replica_communicator",
        "//platform/proto:resdb_cc_proto",
    ],
)
