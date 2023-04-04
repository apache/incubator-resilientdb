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
    name = "in_mem_kv_storage",
    srcs = ["in_mem_kv_storage.cpp"],
    hdrs = ["in_mem_kv_storage.h"],
    deps = [
        ":storage",
        "//common:comm",
    ],
)

cc_test(
    name = "in_mem_kv_storage_test",
    srcs = ["in_mem_kv_storage_test.cpp"],
    deps = [
        ":in_mem_kv_storage",
        "//common/test:test_main",
    ],
)

cc_library(
    name = "res_leveldb",
    srcs = ["res_leveldb.cpp"],
    hdrs = ["res_leveldb.h"],
    deps = [
        ":storage",
        "//common:comm",
        "//platform/proto:replica_info_cc_proto",
        "//third_party:leveldb",
    ],
)

cc_test(
    name = "res_leveldb_test",
    srcs = ["res_leveldb_test.cpp"],
    deps = [
        ":res_leveldb",
        "//common/test:test_main",
    ],
)

cc_library(
    name = "res_rocksdb",
    srcs = ["res_rocksdb.cpp"],
    hdrs = ["res_rocksdb.h"],
    tags = ["manual"],
    deps = [
        ":storage",
        "//common:comm",
        "//platform/proto:replica_info_cc_proto",
        "//third_party:rocksdb",
    ],
)

cc_test(
    name = "res_rocksdb_test",
    srcs = ["res_rocksdb_test.cpp"],
    tags = ["manual"],
    deps = [
        ":res_rocksdb",
        "//common/test:test_main",
    ],
)

cc_library(
    name = "txn_memory_db",
    srcs = ["txn_memory_db.cpp"],
    hdrs = ["txn_memory_db.h"],
    deps = [
        "//common:comm",
        "//platform/proto:resdb_cc_proto",
    ],
)
