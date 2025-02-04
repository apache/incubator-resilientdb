package(default_visibility = ["//visibility:public"])

cc_library(
    name = "generator",
    hdrs = ["generator.h"],
)

cc_library(
    name = "zipfian_generator",
    srcs = ["zipfian_generator.cpp"],
    hdrs = ["zipfian_generator.h"],
    deps = [
        ":generator",
        "//common:comm",
    ],
)

cc_test(
    name = "zipfian_generator_test",
    srcs = ["zipfian_generator_test.cpp"],
    deps = [
        ":zipfian_generator",
        "//common/test:test_main",
    ],
)

cc_library(
    name = "uniform_generator",
    srcs = ["uniform_generator.cpp"],
    hdrs = ["uniform_generator.h"],
    deps = [
        ":generator",
        "//common:comm",
    ],
)

cc_test(
    name = "uniform_generator_test",
    srcs = ["uniform_generator_test.cpp"],
    deps = [
        ":uniform_generator",
        "//common/test:test_main",
    ],
)

cc_library(
    name = "string_generator",
    srcs = ["string_generator.cpp"],
    hdrs = ["string_generator.h"],
    deps = [
        ":generator",
        "//common/utils",
    ],
)

cc_test(
    name = "string_generator_test",
    srcs = ["string_generator_test.cpp"],
    deps = [
        ":string_generator",
        ":zipfian_generator",
        "//common/test:test_main",
    ],
)
