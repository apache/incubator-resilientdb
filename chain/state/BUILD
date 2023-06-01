package(default_visibility = ["//visibility:public"])

cc_library(
    name = "chain_state",
    srcs = ["chain_state.cpp"],
    hdrs = ["chain_state.h"],
    deps = [
        "//storage:storage",
        "//common:comm",
    ],
)

cc_test(
    name = "chain_state_test",
    srcs = ["chain_state_test.cpp"],
    deps = [
        ":chain_state",
        "//common/test:test_main",
    ],
)

