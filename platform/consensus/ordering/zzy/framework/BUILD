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
        "//platform/consensus/ordering/common/framework:consensus",
        "//platform/consensus/ordering/zzy/algorithm:zzy",
    ],
)

cc_library(
    name = "performance_manager",
    srcs = ["performance_manager.cpp"],
    hdrs = ["performance_manager.h"],
    deps = [
        "//platform/consensus/ordering/common/framework:performance_manager",
        "//platform/consensus/ordering/zzy/proto:proposal_cc_proto",
    ],
)

