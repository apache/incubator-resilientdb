package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "pow_server",
    srcs = ["pow_server.cpp"],
    deps = [
        "//platform/config:resdb_config_utils",
        "//platform/consensus/ordering/poc/pow:consensus_service_pow",
        "//platform/statistic:stats",
        "//service/utils:server_factory",
    ],
)
