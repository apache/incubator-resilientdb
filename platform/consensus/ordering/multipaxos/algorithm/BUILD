package(default_visibility = ["//platform/consensus/ordering/multipaxos:__subpackages__"])

cc_library(
    name = "proposal_manager",
    srcs = ["proposal_manager.cpp"],
    hdrs = ["proposal_manager.h"],
    deps = [
        "//common:comm",
        "//common/crypto:signature_verifier",
        "//common/utils",
        "//platform/consensus/ordering/multipaxos/proto:proposal_cc_proto",
    ],
)

cc_library(
    name = "multipaxos",
    srcs = ["multipaxos.cpp"],
    hdrs = ["multipaxos.h"],
    deps = [
        ":proposal_manager",
        "//common:comm",
        "//platform/common/queue:lock_free_queue",
        "//platform/consensus/ordering/common/algorithm:protocol_base",
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
    ],
)

