package(default_visibility = ["//visibility:public"])

cc_library(
    name = "server_factory",
    srcs = ["server_factory.cpp"],
    hdrs = ["server_factory.h"],
    deps = [
        "//platform/config:resdb_config_utils",
        "//platform/consensus/ordering/pbft:consensus_manager_pbft",
        "//platform/networkstrate:service_network",
    ],
)
