package(default_visibility = ["//visibility:private"])

cc_library(
    name = "consensus",
    srcs = ["consensus.cpp"],
    hdrs = ["consensus.h"],
    visibility = [
        "//visibility:public",
    ],
    copts = ["-Iexternal/openenclave"],
    deps = [
        "//common/utils",
        "//platform/consensus/ordering/common/framework:consensus",
        "//platform/consensus/ordering/fides/algorithm:fides",
        "//enclave:headers",
    ],
)

