package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "utxo_server",
    srcs = ["utxo_server.cpp"],
    deps = [
        "//platform/config:resdb_config_utils",
        "//service/utils:server_factory",
        "//service/utxo/executor/service:utxo_transaction_manager",
    ],
)
