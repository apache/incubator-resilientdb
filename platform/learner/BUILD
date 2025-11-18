package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "learner",
    srcs = [
        "learner.cpp",
        "learner_main.cpp",
        "learner.h",
    ],
    deps = [
        "//platform/config:resdb_config_utils",
        "//common:comm",

        "//platform/networkstrate:consensus_manager",
        "//platform/networkstrate:replica_communicator",

        "//platform/proto:resdb_cc_proto",
        "//platform/proto:replica_info_cc_proto",
    ],
)

