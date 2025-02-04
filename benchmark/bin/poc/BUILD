package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "pow_server",
    srcs = ["pow_server.cpp"],
    deps = [
        "//platform/communication:resdb_server",
        "//platform/config:resdb_config_utils",
        "//platform/consensus/ordering/poc/pow:consensus_service_pow",
    ],
)

cc_binary(
    name = "pbft_server",
    srcs = ["pbft_server.cpp"],
    deps = [
        "//platform/communication:resdb_server",
        "//platform/config:resdb_config_utils",
        "//platform/consensus/ordering/pbft:consensus_service_pbft",
    ],
)

cc_library(
    name = "pbft_client",
    srcs = ["pbft_client.cpp"],
    hdrs = ["pbft_client.h"],
    deps = [
        "//platform/consensus/ordering/poc/proto:transaction_cc_proto",
        "//platform/interface:resdb_user_client",
    ],
)

cc_binary(
    name = "pbft_client_main",
    srcs = ["pbft_client_main.cpp"],
    deps = [
        ":pbft_client",
        "//platform/config:resdb_config_utils",
    ],
)
