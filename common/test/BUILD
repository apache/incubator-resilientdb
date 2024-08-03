package(default_visibility = ["//visibility:public"])

cc_library(
    name = "test",
    hdrs = [
        "test_macros.h",
    ],
    deps = [
        "//common:comm",
        "@com_google_googletest//:gtest",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_library(
    name = "test_main",
    defines = ["TEST_MODE"],
    deps = [
        ":test",
        "@com_google_googletest//:gtest_main",
    ],
)

proto_library(
    name = "test_proto",
    srcs = ["test.proto"],
    visibility = ["//visibility:private"],
)

cc_proto_library(
    name = "test_cc_proto",
    visibility = ["//visibility:private"],
    deps = [":test_proto"],
)

cc_test(
    name = "json_test",
    srcs = ["json_test.cpp"],
    deps = [
        ":test_cc_proto",
        ":test_main",
    ],
)
