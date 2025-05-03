package(default_visibility = ["//platform/consensus/ordering/pompe:__subpackages__"])

cc_library(
    name = "proposal_manager",
    srcs = ["proposal_manager.cpp"],
    hdrs = ["proposal_manager.h"],
    deps = [
        "//common:comm",
        "//common/crypto:signature_verifier",
        "//common/utils",
        "//platform/consensus/ordering/pompe/proto:proposal_cc_proto",
    ],
)

cc_library(
    name = "pompe",
    srcs = ["pompe.cpp"],
    hdrs = ["pompe.h"],
    deps = [
        ":proposal_manager",
        "//common/crypto:signature_verifier",
        "//common:comm",
        "//platform/common/queue:lock_free_queue",
        "//platform/consensus/ordering/common/algorithm:protocol_base",
        "//platform/statistic:stats",
    ],
)

cc_library(
    name = "tusk",
    srcs = ["tusk.cpp"],
    hdrs = ["tusk.h"],
    deps = [
        ":proposal_manager",
        "//common/crypto:signature_verifier",
        "//common:comm",
        "//platform/common/queue:lock_free_queue",
        "//platform/consensus/ordering/common/algorithm:protocol_base",
        "//platform/statistic:stats",
    ],
)

