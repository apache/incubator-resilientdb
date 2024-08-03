package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "utxo_service",
    srcs = ["utxo_service.cpp"],
    deps = [
        "//executor/utxo/executor:utxo_executor",
        "//platform/config:resdb_config_utils",
        "//service/utils:server_factory",
    ],
)
