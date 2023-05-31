package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "contract_server",
    srcs = ["contract_server.cpp"],
    deps = [
        "//executor/contract/executor:contract_executor",
        "//platform/config:resdb_config_utils",
        "//platform/consensus/ordering/pbft:consensus_manager_pbft",
        "//server/utils:server_factory",
    ],
)
