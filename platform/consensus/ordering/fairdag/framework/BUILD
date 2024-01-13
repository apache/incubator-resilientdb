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
        "//executor/common:transaction_manager",
        "//platform/consensus/ordering/common/framework:consensus",
        "//platform/consensus/execution:transaction_executor",
        "//platform/consensus/ordering/fairdag/algorithm:fairdag",
        "//platform/networkstrate:consensus_manager",
    ],
)

