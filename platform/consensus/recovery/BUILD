package(default_visibility = ["//platform/consensus:__subpackages__"])

cc_library(
    name = "recovery",
    srcs = ["recovery.cpp"],
    hdrs = ["recovery.h"],
    deps = [
        "//chain/storage",
        "//common/utils",
        "//platform/config:resdb_config",
        "//platform/consensus/checkpoint",
        "//platform/consensus/execution:system_info",
        "//platform/networkstrate:server_comm",
        "//platform/proto:resdb_cc_proto",
        "//platform/proto:system_info_data_cc_proto",
    ],
)

cc_test(
    name = "recovery_test",
    srcs = ["recovery_test.cpp"],
    deps = [
        ":recovery",
        "//chain/storage:mock_storage",
        "//common/test:test_main",
        "//platform/consensus/checkpoint:mock_checkpoint",
        "//platform/consensus/ordering/common:transaction_utils",
    ],
)
