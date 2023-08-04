package(default_visibility = ["//visibility:private"])

cc_library(
    name = "tusk_utils",
    hdrs = ["tusk_utils.h"],
    deps = [
        "//platform/consensus/ordering/tusk/proto:tusk_cc_proto",
        "//platform/proto:resdb_cc_proto",
    ],
)

cc_library(
    name = "message_manager",
    srcs = ["message_manager.cpp"],
    hdrs = ["message_manager.h"],
    deps = [
        ":tusk_utils",
        "//platform/consensus/ordering/common:message_manager_basic",
        "//platform/consensus/ordering/tusk/proto:tusk_cc_proto",
    ],
)

cc_library(
    name = "commitment",
    srcs = ["commitment.cpp"],
    hdrs = ["commitment.h"],
    deps = [
        ":message_manager",
        ":tusk_utils",
        "//platform/consensus/ordering/common:commitment_basic",
        "//platform/networkstrate:replica_communicator",
    ],
)

cc_library(
    name = "response_manager",
    srcs = ["response_manager.cpp"],
    hdrs = ["response_manager.h"],
    deps = [
        "//platform/consensus/ordering/tusk/proto:tusk_cc_proto",
        "//platform/networkstrate:replica_communicator",
        "//platform/networkstrate:server_comm",
    ],
)

cc_library(
    name = "performance_manager",
    srcs = ["performance_manager.cpp"],
    hdrs = ["performance_manager.h"],
    deps = [
        "//platform/consensus/ordering/common:performance_generator",
        "//platform/consensus/ordering/tusk/proto:tusk_cc_proto",
        "//platform/networkstrate:replica_communicator",
        "//platform/networkstrate:server_comm",
    ],
)

cc_library(
    name = "consensus_manager_tusk",
    srcs = ["consensus_manager_tusk.cpp"],
    hdrs = ["consensus_manager_tusk.h"],
    visibility = ["//visibility:public"],
    deps = [
        ":commitment",
        ":message_manager",
        ":performance_manager",
        ":response_manager",
        "//common/crypto:signature_verifier",
        "//platform/networkstrate:consensus_manager",
    ],
)
