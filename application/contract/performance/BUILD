package(default_visibility = ["//visibility:public"])

cc_test(
    name = "performance_test",
    srcs = ["performance_test.cpp"],
    data = [
        "//application/contract/performance/data:kv.json",
    ],
    deps = [
        "//application/contract/service:contract_executor",
        "//common/test:test_main",
    ],
)
