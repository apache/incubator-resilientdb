package(default_visibility = ["//visibility:public"])

cc_test(
    name = "performance_test",
    srcs = ["performance_test.cpp"],
    data = [
        "//service/contract/performance/data:kv.json",
    ],
    tags = ["manual"],
    deps = [
        "//common/test:test_main",
        "//service/contract/executor/service:contract_executor",
    ],
)
