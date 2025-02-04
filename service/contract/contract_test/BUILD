package(default_visibility = ["//visibility:private"])

cc_binary(
    name = "tpcc_test",
    srcs = ["tpcc_test.cpp"],
    data = [
        "//service/contract/benchmark/data:tpcc.json",
    ],
    deps = [
        "//common/utils",
        "//service/contract/executor/x_manager:contract_manager",
    ],
)
