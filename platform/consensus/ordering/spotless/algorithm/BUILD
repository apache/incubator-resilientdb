package(default_visibility = ["//platform/consensus/ordering/spotless:__subpackages__"])

cc_library(
    name = "proposal_manager",
    srcs = ["proposal_manager.cpp"],
    hdrs = ["proposal_manager.h"],
    deps = [
        "//common:comm",
        "//common/crypto:signature_verifier",
        "//common/utils",
        "//platform/consensus/ordering/spotless/proto:proposal_cc_proto",
    ],
)

cc_library(
    name = "spotless",
    srcs = ["spotless.cpp"],
    hdrs = ["spotless.h"],
    deps = [
        ":proposal_manager",
        "//common:comm",
        "//platform/common/queue:lock_free_queue",
        "//platform/consensus/ordering/common/algorithm:protocol_base",
        "//platform/statistic:stats",
    ],
)
