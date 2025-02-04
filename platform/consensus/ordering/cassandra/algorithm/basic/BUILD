package(default_visibility = ["//platform/consensus/ordering/cassandra:__subpackages__"])

cc_library(
    name = "miner_utils",
    srcs = ["miner_utils.cpp"],
    hdrs = ["miner_utils.h"],
    deps = [
        "//common:comm",
        "//common/crypto:signature_verifier",
        "//platform/consensus/ordering/cassandra/proto:proposal_cc_proto",
    ],
)

cc_library(
    name = "miner",
    srcs = ["miner.cpp"],
    hdrs = ["miner.h"],
    deps = [
        ":miner_utils",
        "//common:comm",
        "//platform/consensus/ordering/cassandra/proto:proposal_cc_proto",
    ],
)

cc_library(
    name = "proposal_manager",
    srcs = ["proposal_manager.cpp"],
    hdrs = ["proposal_manager.h"],
    deps = [
        ":miner",
        ":proposal_graph",
        "//common:comm",
        "//common/crypto:signature_verifier",
        "//common/utils",
        "//platform/consensus/ordering/cassandra/proto:proposal_cc_proto",
    ],
)

cc_library(
    name = "ranking",
    srcs = ["ranking.cpp"],
    hdrs = ["ranking.h"],
    deps = [
        "//common:comm",
    ],
)

cc_library(
    name = "proposal_graph",
    srcs = ["proposal_graph.cpp"],
    hdrs = ["proposal_graph.h"],
    deps = [
        ":ranking",
        "//common:comm",
        "//platform/consensus/ordering/cassandra/algorithm/comm:proposal_state",
        "//platform/consensus/ordering/cassandra/proto:proposal_cc_proto",
    ],
)

cc_library(
    name = "cassandra",
    srcs = ["cassandra.cpp"],
    hdrs = ["cassandra.h"],
    deps = [
        ":proposal_graph",
        ":proposal_manager",
        "//common:comm",
        "//common/crypto:signature_verifier",
        "//platform/common/queue:lock_free_queue",
    ],
)

cc_library(
    name = "proposal_manager_mem",
    srcs = ["proposal_manager_mem.cpp"],
    hdrs = ["proposal_manager_mem.h"],
    deps = [
        ":miner",
        ":proposal_graph",
        "//common:comm",
        "//common/crypto:signature_verifier",
        "//common/utils",
        "//platform/consensus/ordering/cassandra/proto:proposal_cc_proto",
    ],
)

cc_library(
    name = "cassandra_mem",
    srcs = ["cassandra_mem.cpp"],
    hdrs = ["cassandra.h"],
    deps = [
        ":proposal_graph",
        ":proposal_manager_mem",
        "//common:comm",
        "//common/crypto:signature_verifier",
        "//platform/common/queue:lock_free_queue",
    ],
)

cc_test(
    name = "proposal_graph_test",
    srcs = ["proposal_graph_test.cpp"],
    deps = [
        ":proposal_graph",
        "//common/test:test_main",
    ],
)

cc_test(
    name = "cassandra_test",
    srcs = ["cassandra_test.cpp"],
    deps = [
        ":cassandra",
        "//common/test:test_main",
    ],
)
