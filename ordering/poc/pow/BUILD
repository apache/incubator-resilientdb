package(default_visibility = ["//visibility:public"])

cc_library(
    name = "miner_utils",
    srcs = ["miner_utils.cpp"],
    hdrs = ["miner_utils.h"],
    deps = [
        "//crypto:signature_verifier",
        "//ordering/poc/proto:pow_cc_proto",
        "@boost//:format",
    ],
)

cc_test(
    name = "miner_utils_test",
    srcs = ["miner_utils_test.cpp"],
    deps = [
        ":miner_utils",
        "//common/test:test_main",
        "//config:resdb_config_utils",
        "//crypto:signature_verifier",
    ],
)

cc_library(
    name = "merkle",
    srcs = ["merkle.cpp"],
    hdrs = ["merkle.h"],
    deps = [
        ":miner_utils",
    ],
)

cc_test(
    name = "merkle_test",
    srcs = ["merkle_test.cpp"],
    deps = [
        ":merkle",
        "//common/test:test_main",
    ],
)

cc_library(
    name = "miner",
    srcs = ["miner.cpp"],
    hdrs = ["miner.h"],
    deps = [
        ":miner_utils",
        "//config:resdb_poc_config",
        "//crypto:signature_verifier",
        "//ordering/poc/proto:pow_cc_proto",
        "@boost//:format",
    ],
)

cc_test(
    name = "miner_test",
    srcs = ["miner_test.cpp"],
    deps = [
        ":miner",
        "//common/test:test_main",
        "//config:resdb_config_utils",
    ],
)

cc_library(
    name = "miner_manager",
    srcs = ["miner_manager.cpp"],
    hdrs = ["miner_manager.h"],
    deps = [
        "//config:resdb_poc_config",
    ],
)

cc_library(
    name = "block_manager",
    srcs = ["block_manager.cpp"],
    hdrs = ["block_manager.h"],
    deps = [
        ":merkle",
        ":miner",
        "//common:comm",
        "//config:resdb_poc_config",
        "//ordering/poc/proto:pow_cc_proto",
        "//statistic:stats",
    ],
)

cc_test(
    name = "block_manager_test",
    srcs = ["block_manager_test.cpp"],
    deps = [
        ":block_manager",
        "//common/test:test_main",
        "//config:resdb_config_utils",
        "//crypto:key_generator",
    ],
)

cc_library(
    name = "transaction_accessor",
    srcs = ["transaction_accessor.cpp"],
    hdrs = ["transaction_accessor.h"],
    deps = [
        ":miner_manager",
        "//client:resdb_txn_client",
        "//common/queue:lock_free_queue",
        "//config:resdb_poc_config",
        "//ordering/poc/proto:pow_cc_proto",
    ],
)

cc_test(
    name = "transaction_accessor_test",
    srcs = ["transaction_accessor_test.cpp"],
    deps = [
        ":transaction_accessor",
        "//client:mock_resdb_client",
        "//client:mock_resdb_txn_client",
        "//common/test:test_main",
        "//config:resdb_config_utils",
    ],
)

cc_library(
    name = "consensus_service_pow",
    srcs = ["consensus_service_pow.cpp"],
    hdrs = ["consensus_service_pow.h"],
    deps = [
        ":block_manager",
        ":transaction_accessor",
        "//common/queue:blocking_queue",
        "//common/utils",
        "//ordering/poc/proto:pow_cc_proto",
        "//server:consensus_service",
    ],
)

cc_test(
    name = "consensus_service_pow_test",
    srcs = ["consensus_service_pow_test.cpp"],
    deps = [
        ":consensus_service_pow",
        "//common/test:test_main",
        "//config:resdb_config_utils",
        "//crypto:key_generator",
        "//statistic:stats",
    ],
)
