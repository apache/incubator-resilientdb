package(default_visibility = ["//platform/consensus/ordering/zzy:__subpackages__"])

cc_library(
    name = "zzy",
    srcs = ["zzy.cpp"],
    hdrs = ["zzy.h"],
    deps = [
        "//common:comm",
        "//common/crypto:signature_verifier",
        "//platform/common/queue:lock_free_queue",
        "//platform/config:resdb_config",
        "//platform/consensus/ordering/common/algorithm:protocol_base",
        "//platform/consensus/ordering/zzy/proto:proposal_cc_proto",
        "//platform/statistic:stats",
    ],
)
