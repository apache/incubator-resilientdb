package(default_visibility = ["//visibility:public"])

cc_library(
    name = "resdb_config",
    srcs = ["resdb_config.cpp"],
    hdrs = ["resdb_config.h"],
    deps = [
        "//common:comm",
        "//platform/proto:replica_info_cc_proto",
    ],
)

cc_test(
    name = "resdb_config_test",
    srcs = ["resdb_config_test.cpp"],
    deps = [
        ":resdb_config",
        "//common/test:test_main",
    ],
)

cc_library(
    name = "resdb_config_utils",
    srcs = ["resdb_config_utils.cpp"],
    hdrs = ["resdb_config_utils.h"],
    deps = [
        ":resdb_config",
    ],
)

cc_library(
    name = "resdb_poc_config",
    srcs = ["resdb_poc_config.cpp"],
    hdrs = ["resdb_poc_config.h"],
    deps = [
        ":resdb_config",
    ],
)
