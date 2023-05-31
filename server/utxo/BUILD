package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "utxo_server",
    srcs = ["utxo_server.cpp"],
    deps = [
        "//executor/utxo/executor:utxo_executor",
        "//platform/config:resdb_config_utils",
        "//server/utils:server_factory",
    ],
)
