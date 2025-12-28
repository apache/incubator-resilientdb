package(default_visibility = ["//visibility:public"])

cc_library(
    name = "miner_utils",
    srcs = ["miner_utils.cpp"],
    hdrs = ["miner_utils.h"],
    deps = [
        "//common/crypto:signature_verifier",
        "//platform/consensus/ordering/poc/proto:pow_cc_proto",
        "@boost//:format",
    ],
)

cc_test(
    name = "miner_utils_test",
    srcs = ["miner_utils_test.cpp"],
    deps = [
        ":miner_utils",
        "//common/test:test_main",
        "//platform/config:resdb_config_utils",
        "//common/crypto:signature_verifier",
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
        "//platform/config:resdb_poc_config",
        "//common/crypto:signature_verifier",
        "//platform/consensus/ordering/poc/proto:pow_cc_proto",
        "@boost//:format",
    ],
)

cc_test(
    name = "miner_test",
    srcs = ["miner_test.cpp"],
    deps = [
        ":miner",
        "//common/test:test_main",
        "//platform/config:resdb_config_utils",
    ],
)

cc_library(
    name = "miner_manager",
    srcs = ["miner_manager.cpp"],
    hdrs = ["miner_manager.h"],
    deps = [
        "//platform/config:resdb_poc_config",
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
        "//platform/config:resdb_poc_config",
        "//platform/consensus/ordering/poc/proto:pow_cc_proto",
        "//platform/statistic:stats",
        "//platform/proto:resdb_cc_proto",
    ],
)

cc_test(
    name = "block_manager_test",
    srcs = ["block_manager_test.cpp"],
    deps = [
        ":block_manager",
        "//common/test:test_main",
        "//platform/config:resdb_config_utils",
        "//common/crypto:key_generator",
    ],
)

cc_library(
    name = "transaction_accessor",
    srcs = ["transaction_accessor.cpp"],
    hdrs = ["transaction_accessor.h"],
    deps = [
        ":miner_manager",
        "//interface/common:resdb_txn_accessor",
        "//platform/common/queue:lock_free_queue",
        "//common/utils:utils",
        "//platform/config:resdb_poc_config",
        "//platform/consensus/ordering/poc/proto:pow_cc_proto",
        "//platform/statistic:stats",
    ],
)

cc_library(
    name = "mock_transaction_accessor",
    hdrs = ["mock_transaction_accessor.h"],
    deps = [
        ":transaction_accessor",
    ],
)

cc_test(
    name = "transaction_accessor_test",
    srcs = ["transaction_accessor_test.cpp"],
    deps = [
        ":transaction_accessor",
        "//interface/rdbc:net_channel",
        "//interface/common:mock_resdb_txn_accessor",
        "//common/test:test_main",
        "//platform/config:resdb_config_utils",
    ],
)

cc_library(
    name = "consensus_service_pow",
    srcs = ["consensus_service_pow.cpp"],
    hdrs = ["consensus_service_pow.h"],
    deps = [
        ":block_manager",
        ":transaction_accessor",
        ":pow_manager",
        "//platform/common/queue:blocking_queue",
        "//common/utils",
        "//platform/consensus/ordering/poc/proto:pow_cc_proto",
        "//platform/networkstrate:service_network",
        "//platform/consensus/ordering/common/framework:consensus"
    ],
)

'''
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
'''

cc_library(
    name = "shift_manager",
    srcs = ["shift_manager.cpp"],
    hdrs = ["shift_manager.h"],
    deps = [
        "//platform/config:resdb_poc_config",
        "//platform/consensus/ordering/poc/proto:pow_cc_proto",
    ],
)


cc_library(
    name = "pow_manager",
    srcs = ["pow_manager.cpp"],
    hdrs = ["pow_manager.h"],
    deps = [
        ":block_manager",
        ":shift_manager",
        ":transaction_accessor",
        "//platform/networkstrate:replica_communicator",
        "//platform/common/queue:blocking_queue",
        "//platform/consensus/ordering/poc/proto:pow_cc_proto",
        "//common/utils",
    ],
)

cc_test(
    name = "pow_manager_test",
    srcs = ["pow_manager_test.cpp"],
    deps = [
      ":pow_manager",
      ":mock_transaction_accessor",
        "//platform/networkstrate:mock_replica_communicator",
      "//common/test:test_main",
      "//platform/config:resdb_config_utils",
    ],
)
