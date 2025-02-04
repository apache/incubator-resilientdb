package(default_visibility = ["//visibility:public"])

cc_library(
    name = "contract_transaction_manager",
    srcs = ["contract_transaction_manager.cpp"],
    hdrs = ["contract_transaction_manager.h"],
    deps = [
        "//platform/config:resdb_config_utils",
        "//service/contract/executor/manager:address_manager",
        "//service/contract/executor/manager:contract_manager",
        "//service/contract/proto:rpc_cc_proto",
        "//service/utils:server_factory",
    ],
)

cc_test(
    name = "contract_transaction_manager_test",
    srcs = ["contract_transaction_manager_test.cpp"],
    data = [
        "//service/contract/executor/service/test_data:contract.json",
    ],
    deps = [
        ":contract_transaction_manager",
        "//common/test:test_main",
    ],
)
