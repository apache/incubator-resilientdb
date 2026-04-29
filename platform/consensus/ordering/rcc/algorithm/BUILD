package(default_visibility = ["//platform/consensus/ordering/rcc:__subpackages__"])

cc_library(
    name = "proposal_manager",
    srcs = ["proposal_manager.cpp"],
    hdrs = ["proposal_manager.h"],
    deps = [
        "//common:comm",
        "//common/crypto:signature_verifier",
        "//common/utils",
        "//platform/consensus/ordering/rcc/proto:proposal_cc_proto",
    ],
)

cc_library(
    name = "rcc",
    srcs = ["rcc.cpp"],
    hdrs = ["rcc.h"],
    deps = [
        ":proposal_manager",
        ":transaction_collector",
        "//common:comm",
        "//platform/common/queue:lock_free_queue",
        "//platform/consensus/ordering/common/algorithm:protocol_base",
    ],
)

cc_library(
    name = "transaction_collector",
    srcs = ["transaction_collector.cpp"],
    hdrs = ["transaction_collector.h"],
    deps = [
        "//platform/statistic:stats",
    ],
)
