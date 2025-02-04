package(default_visibility = ["//visibility:public"])

cc_library(
    name = "contract_executor",
    srcs = ["contract_executor.cpp"],
    hdrs = ["contract_executor.h"],
    deps = [
        "//executor/common:transaction_manager",
        "//platform/config:resdb_config_utils",
        "//proto/contract:rpc_cc_proto",
        "//service/contract/executor/x_manager:contract_manager",
        "//service/contract/proto:rpc_cc_proto",
    ],
)

cc_test(
    name = "contract_executor_test",
    srcs = ["contract_executor_test.cpp"],
    data = [
        "//executor/contract/executor/test_data:contract.json",
    ],
    tags = ["manual"],
    deps = [
        ":contract_executor",
        "//common/test:test_main",
    ],
)
