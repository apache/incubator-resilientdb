package(default_visibility = ["//visibility:public"])

load("@rules_foreign_cc//foreign_cc:defs.bzl", "cmake")

cc_library(
    name = "eEVM",
    srcs = glob(
        ["**/*.cpp"],
        exclude = [
            "3rdparty/intx/test/**",
            "3rdparty/intx/examples/**",
            "samples/**",
            "tests/**",
        ],
    )+ glob(
        ["3rdparty/keccak/*.c"] 
    ),
    hdrs = glob(
        ["**/*.hpp"],
    ) + glob(
        ["**/*.h"],
    ) + glob(
        ["3rdparty/keccak/*.macros"]
    ) + glob(
        ["3rdparty/keccak/*.inc"]
    ),
    includes = [
        ".",
        "3rdparty",
        "include",
        "3rdparty/intx/include",
        "3rdparty/keccak",
    ],
    #linkstatic = 1,
    visibility = ["//visibility:public"],
)
