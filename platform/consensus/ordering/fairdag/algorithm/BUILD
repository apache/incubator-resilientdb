package(default_visibility = ["//platform/consensus/ordering/fairdag:__subpackages__"])

cc_library(
    name = "proposal_manager",
    srcs = ["proposal_manager.cpp"],
    hdrs = ["proposal_manager.h"],
    deps = [
        "//common:comm",
        "//common/crypto:signature_verifier",
        "//common/utils",
        "//platform/consensus/ordering/fairdag/proto:proposal_cc_proto",
    ],
)

cc_library(
    name = "fairdag",
    srcs = ["fairdag.cpp"],
    hdrs = ["fairdag.h"],
    deps = [
        ":proposal_manager",
        "//common/crypto:signature_verifier",
        "//common:comm",
        "//platform/common/queue:lock_free_queue",
        "//platform/consensus/ordering/common/algorithm:protocol_base",
    ],
)

