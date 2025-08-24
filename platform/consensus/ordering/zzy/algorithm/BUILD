package(default_visibility = ["//platform/consensus/ordering/zzy:__subpackages__"])

cc_library(
    name = "zzy",
    srcs = ["zzy.cpp"],
    hdrs = ["zzy.h"],
    deps = [
        "//platform/statistic:stats",
        "//common:comm",
        "//platform/consensus/ordering/zzy/proto:proposal_cc_proto",
        "//common/crypto:signature_verifier",
        "//platform/common/queue:lock_free_queue",
        "//platform/config:resdb_config",
      "//platform/consensus/ordering/common/algorithm:protocol_base",
    ],
)
