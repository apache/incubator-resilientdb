package(default_visibility = ["//visibility:public"])

cc_library(
    name = "resdb_client",
    srcs = ["resdb_client.cpp"],
    hdrs = ["resdb_client.h"],
    deps = [
        "//common:comm",
        "//common/data_comm",
        "//common/network:tcp_socket",
        "//config:resdb_config",
        "//crypto:signature_verifier",
        "//proto:resdb_cc_proto",
    ],
)

cc_library(
    name = "mock_resdb_client",
    testonly = True,
    hdrs = ["mock_resdb_client.h"],
    deps = [
        ":resdb_client",
    ],
)

cc_test(
    name = "resdb_client_test",
    srcs = ["resdb_client_test.cpp"],
    deps = [
        ":resdb_client",
        "//common/network:mock_socket",
        "//common/test:test_main",
        "//crypto:key_generator",
        "//proto:client_test_cc_proto",
    ],
)

cc_library(
    name = "resdb_user_client",
    srcs = ["resdb_user_client.cpp"],
    hdrs = ["resdb_user_client.h"],
    deps = [
        ":resdb_client",
        "//common/data_comm",
    ],
)

cc_test(
    name = "resdb_user_client_test",
    srcs = ["resdb_user_client_test.cpp"],
    deps = [
        ":resdb_user_client",
        "//common/network:mock_socket",
        "//common/test:test_main",
        "//crypto:signature_verifier",
        "//proto:client_test_cc_proto",
    ],
)

cc_library(
    name = "resdb_state_client",
    srcs = ["resdb_state_client.cpp"],
    hdrs = ["resdb_state_client.h"],
    deps = [
        ":resdb_user_client",
        "//common:comm",
        "//proto:replica_info_cc_proto",
        "//proto:resdb_cc_proto",
    ],
)

cc_test(
    name = "resdb_state_client_test",
    srcs = ["resdb_state_client_test.cpp"],
    deps = [
        ":mock_resdb_client",
        ":resdb_state_client",
        "//common/test:test_main",
    ],
)

cc_library(
    name = "resdb_txn_client",
    srcs = ["resdb_txn_client.cpp"],
    hdrs = ["resdb_txn_client.h"],
    deps = [
        ":resdb_client",
        "//common:comm",
        "//proto:replica_info_cc_proto",
        "//proto:resdb_cc_proto",
    ],
)

cc_library(
    name = "mock_resdb_txn_client",
    testonly = True,
    hdrs = ["mock_resdb_txn_client.h"],
    deps = [
        ":resdb_txn_client",
    ],
)

cc_test(
    name = "resdb_txn_client_test",
    srcs = ["resdb_txn_client_test.cpp"],
    deps = [
        ":mock_resdb_client",
        ":resdb_txn_client",
        "//common/test:test_main",
        "//config:resdb_config_utils",
    ],
)
