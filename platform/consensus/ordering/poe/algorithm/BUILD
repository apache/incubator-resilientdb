package(default_visibility = ["//platform/consensus/ordering/poe:__subpackages__"])

cc_library(
    name = "poe",
    srcs = ["poe.cpp"],
    hdrs = ["poe.h"],
    deps = [
        "//platform/statistic:stats",
        "//common:comm",
        "//platform/consensus/ordering/poe/proto:proposal_cc_proto",
        "//common/crypto:signature_verifier",
        "//platform/consensus/ordering/common/algorithm:protocol_base",
        "//platform/common/queue:lock_free_queue",
        "//platform/config:resdb_config",
    ],
)
