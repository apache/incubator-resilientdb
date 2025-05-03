package(default_visibility = ["//visibility:public"])

cc_library(
    name = "storage",
    hdrs = ["storage.h"],
    deps = [
    ],
)

cc_library(
    name = "mock_storage",
    hdrs = ["mock_storage.h"],
    deps = [
        ":storage",
    ],
)

cc_library(
    name = "memory_db",
    srcs = ["memory_db.cpp"],
    hdrs = ["memory_db.h"],
    deps = [
        ":storage",
        "//common:comm",
    ],
)

cc_library(
    name = "leveldb",
    srcs = ["leveldb.cpp"],
    hdrs = ["leveldb.h"],
    deps = [
        ":storage",
        "//chain/storage/proto:kv_cc_proto",
        "//chain/storage/proto:leveldb_config_cc_proto",
        "//common:comm",
        "//third_party:leveldb",
    ],
)

cc_library(
    name = "rocksdb",
    srcs = ["rocksdb.cpp"],
    hdrs = ["rocksdb.h"],
    tags = ["manual"],
    deps = [
        ":storage",
        "//chain/storage/proto:kv_cc_proto",
        "//chain/storage/proto:rocksdb_config_cc_proto",
        "//common:comm",
        "//third_party:rocksdb",
    ],
)

cc_test(
    name = "kv_storage_test",
    srcs = ["kv_storage_test.cpp"],
    deps = [
        ":leveldb",
        ":memory_db",
        ":rocksdb",
        "//common/test:test_main",
    ],
)
