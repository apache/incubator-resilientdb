package(default_visibility = ["//visibility:public"])

cc_library(
    name = "test",
    hdrs = [
        "test_macros.h",
    ],
    deps = [
        "@com_google_googletest//:gtest",
    ],
)

cc_library(
    name = "test_main",
    deps = [
        ":test",
        "@com_google_googletest//:gtest_main",
    ],
)
