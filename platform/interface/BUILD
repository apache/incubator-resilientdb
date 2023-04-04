package(default_visibility = ["//visibility:public"])

cc_library(
    name = "resdb_net_channel",
    srcs = ["resdb_net_channel.cpp"],
    hdrs = ["resdb_net_channel.h"],
    visibility = ["//platform/communication:__pkg__"],
    deps = [
        "//common:comm",
        "//common/crypto:signature_verifier_interface",
        "//platform/common/data_comm",
        "//platform/common/network:tcp_socket",
        "//platform/config:resdb_config",
        "//platform/proto:resdb_cc_proto",
    ],
)

cc_library(
    name = "mock_resdb_net_channel",
    testonly = True,
    hdrs = ["mock_resdb_net_channel.h"],
    deps = [
        ":resdb_net_channel",
    ],
)

cc_test(
    name = "resdb_net_channel_test",
    srcs = ["resdb_net_channel_test.cpp"],
    deps = [
        ":resdb_net_channel",
        "//:cryptopp_lib",
        "//common/crypto:key_generator",
        "//common/test:test_main",
        "//common/crypto:signature_verifier",
        "//platform/common/network:mock_socket",
        "//platform/proto:client_test_cc_proto",
    ],
)

cc_library(
    name = "resdb_transaction_proposer",
    srcs = ["resdb_transaction_proposer.cpp"],
    hdrs = ["resdb_transaction_proposer.h"],
    deps = [
        ":resdb_net_channel",
        "//platform/common/data_comm",
    ],
)

cc_test(
    name = "resdb_transaction_proposer_test",
    srcs = ["resdb_transaction_proposer_test.cpp"],
    deps = [
        ":resdb_transaction_proposer",
        "//common/crypto:signature_verifier",
        "//common/test:test_main",
        "//platform/common/network:mock_socket",
        "//platform/proto:client_test_cc_proto",
    ],
)

cc_library(
    name = "resdb_state_accessor",
    srcs = ["resdb_state_accessor.cpp"],
    hdrs = ["resdb_state_accessor.h"],
    deps = [
        ":resdb_net_channel",
        "//common:comm",
        "//platform/proto:replica_info_cc_proto",
        "//platform/proto:resdb_cc_proto",
    ],
)

cc_test(
    name = "resdb_state_accessor_test",
    srcs = ["resdb_state_accessor_test.cpp"],
    deps = [
        ":mock_resdb_net_channel",
        ":resdb_state_accessor",
        "//common/test:test_main",
    ],
)

cc_library(
    name = "resdb_txn_accessor",
    srcs = ["resdb_txn_accessor.cpp"],
    hdrs = ["resdb_txn_accessor.h"],
    deps = [
        ":resdb_net_channel",
        "//common:comm",
        "//platform/proto:replica_info_cc_proto",
        "//platform/proto:resdb_cc_proto",
    ],
)

cc_library(
    name = "mock_resdb_txn_accessor",
    testonly = True,
    hdrs = ["mock_resdb_txn_accessor.h"],
    deps = [
        ":resdb_txn_accessor",
    ],
)

cc_test(
    name = "resdb_txn_accessor_test",
    srcs = ["resdb_txn_accessor_test.cpp"],
    deps = [
        ":mock_resdb_net_channel",
        ":resdb_txn_accessor",
        "//common/test:test_main",
        "//platform/config:resdb_config_utils",
    ],
)
