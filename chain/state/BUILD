package(default_visibility = ["//visibility:public"])

cc_library(
    name = "chain_state",
    srcs = ["chain_state.cpp"],
    hdrs = ["chain_state.h"],
    deps = [
        "//chain/storage",
        "//common:comm",
    ],
)

cc_test(
    name = "chain_state_test",
    srcs = ["chain_state_test.cpp"],
    deps = [
        ":chain_state",
        "//common/test:test_main",
    ],
)

cc_library(
    name = "chain_state_utils",
    srcs = ["chain_state_utils.cpp"],
    hdrs = ["chain_state_utils.h"],
    copts = select({
        "//executor/kv:enable_leveldb_setting": ["-DENABLE_LEVELDB"],
        "//executor/kv:enable_rocksdb_setting": ["-DENABLE_ROCKSDB"],
        "//conditions:default": [],
    }),
    deps = [
        ":chain_state",
        "//platform/config:resdb_config_utils",
    ] + select({
        "//executor/kv:enable_leveldb_setting": ["//chain/storage:res_leveldb"],
        "//executor/kv:enable_rocksdb_setting": ["//storage:res_rocksdb"],
        "//conditions:default": [],
    }),
)
