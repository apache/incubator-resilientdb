package(default_visibility = ["//visibility:public"])

cc_library(
    name = "logging",
    srcs = [
        "logging.cpp",
    ],
    hdrs = [
        "logging.h",
    ],
    deps = [
        "//common:comm",
        "//proto:logging_cc_proto",
    ],
)

cc_test(
    name = "logging_test",
    srcs = ["logging_test.cpp"],
    deps = [
        ":logging",
        "//common/test",
        "//common/test:test_main",
        "//proto:logging_cc_proto",
    ],
)
