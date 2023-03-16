package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "contract_server",
    srcs = ["contract_server.cpp"],
    deps = [
        "//platform/config:resdb_config_utils",
        "//platform/consensus/ordering/pbft:consensus_manager_pbft",
        "//service/contract/executor/service:contract_transaction_manager",
        "//service/utils:server_factory",
    ],
)
