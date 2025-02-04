package(default_visibility = ["//visibility:private"])

cc_library(
    name = "consensus",
    srcs = ["consensus.cpp"],
    hdrs = ["consensus.h"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":performance_manager",
        "//common/utils",
        "//executor/common:transaction_manager",
        "//platform/consensus/execution:transaction_executor",
        "//platform/consensus/ordering/common/framework:consensus",
        "//platform/consensus/ordering/fairdag/algorithm:fairdag",
        "//platform/networkstrate:consensus_manager",
    ],
)

cc_library(
    name = "performance_manager",
    srcs = ["performance_manager.cpp"],
    hdrs = ["performance_manager.h"],
    deps = [
        "//platform/consensus/ordering/common/framework:performance_manager",
    ],
)
