package(default_visibility = ["//platform/consensus/ordering/slot_hs1:__subpackages__"])

cc_library(
    name = "proposal_manager",
    srcs = ["proposal_manager.cpp"],
    hdrs = ["proposal_manager.h"],
    deps = [
        "//common:comm",
        "//common/crypto:signature_verifier",
        "//common/utils",
        "//platform/consensus/ordering/slot_hs1/proto:proposal_cc_proto",
        "//platform/statistic:stats",
    ],
)

cc_library(
    name = "slot_hs1",
    srcs = ["slot_hs1.cpp"],
    hdrs = ["slot_hs1.h"],
    deps = [
        ":proposal_manager",
        "//common:comm",
        "//platform/common/queue:lock_free_queue",
        "//platform/consensus/ordering/common/algorithm:protocol_base",
        "//platform/statistic:stats",
    ],
)

