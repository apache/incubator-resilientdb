package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "contract_service",
    srcs = ["contract_service.cpp"],
    deps = [
        "//executor/contract/executor:contract_executor",
        "//platform/config:resdb_config_utils",
        "//platform/consensus/ordering/pbft:consensus_manager_pbft",
        "//service/utils:server_factory",
    ],
)
