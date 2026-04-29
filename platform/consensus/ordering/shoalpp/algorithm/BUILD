package(default_visibility = ["//platform/consensus/ordering/shoalpp:__subpackages__"])

cc_library(
    name = "proposal_manager",
    srcs = ["proposal_manager.cpp"],
    hdrs = ["proposal_manager.h"],
    deps = [
        "//common:comm",
        "//common/crypto:signature_verifier",
        "//common/utils",
        "//platform/consensus/ordering/shoalpp/proto:proposal_cc_proto",
    ],
)

cc_library(
    name = "shoalpp",
    srcs = ["shoalpp.cpp"],
    hdrs = ["shoalpp.h"],
    deps = [
        ":proposal_manager",
        "//common:comm",
        "//common/crypto:signature_verifier",
        "//platform/common/queue:lock_free_queue",
        "//platform/consensus/ordering/common/algorithm:protocol_base",
        "//platform/statistic:stats",
        "//platform/config:resdb_config",
    ],
)
