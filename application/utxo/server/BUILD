package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "utxo_server",
    srcs = ["utxo_server.cpp"],
    deps = [
        "//application/utils:server_factory",
        "//application/utxo/service:utxo_executor",
        "//config:resdb_config_utils",
        "//ordering/pbft:consensus_service_pbft",
    ],
)
