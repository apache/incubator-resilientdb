package(default_visibility = ["//visibility:private"])

cc_library(
    name = "consensus",
    srcs = ["consensus.cpp"],
    hdrs = ["consensus.h"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//common/utils",
        "//platform/consensus/ordering/common/framework:consensus",
        "//platform/consensus/ordering/simple_pbft/algorithm:pbft",
    ],
)
