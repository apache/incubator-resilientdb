package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "contract_server",
    srcs = ["contract_server.cpp"],
    deps = [
        "//application/contract/service:contract_executor",
        "//application/utils:server_factory",
        "//config:resdb_config_utils",
        "//ordering/pbft:consensus_service_pbft",
    ],
)
