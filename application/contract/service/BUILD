package(default_visibility = ["//visibility:public"])

cc_library(
    name = "contract_executor",
    srcs = ["contract_executor.cpp"],
    hdrs = ["contract_executor.h"],
    deps = [
        "//application/contract/manager:address_manager",
        "//application/contract/manager:contract_manager",
        "//application/contract/proto:rpc_cc_proto",
        "//application/utils:server_factory",
        "//config:resdb_config_utils",
        "//ordering/pbft:consensus_service_pbft",
    ],
)

cc_test(
    name = "contract_executor_test",
    srcs = ["contract_executor_test.cpp"],
    data = [
        "//application/contract/service/test_data:contract.json",
    ],
    deps = [
        ":contract_executor",
        "//common/test:test_main",
    ],
)
