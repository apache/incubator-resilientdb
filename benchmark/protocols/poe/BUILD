package(default_visibility = ["//visibility:private"])

load("@bazel_skylib//rules:common_settings.bzl", "bool_flag")

cc_binary(
    name = "kv_server_performance",
    srcs = ["kv_server_performance.cpp"],
    deps = [
<<<<<<< HEAD
        "//chain/storage:memory_db",
        "//executor/kv:kv_executor",
        "//platform/config:resdb_config_utils",
        "//platform/consensus/ordering/poe/framework:consensus",
=======
        "//executor/kv:kv_executor",
        "//platform/config:resdb_config_utils",
        "//platform/consensus/ordering/poe/mac:consensus",
>>>>>>> 4b50ac6c0980c0650ac2a24d84a901b36ce2298c
        "//service/utils:server_factory",
    ],
)

<<<<<<< HEAD
=======
cc_binary(
    name = "kv_server_performance_nomac",
    srcs = ["kv_server_performance_nomac.cpp"],
    deps = [
        "//executor/kv:kv_executor",
        "//platform/config:resdb_config_utils",
        "//platform/consensus/ordering/poe/nomac:consensus",
        "//service/utils:server_factory",
    ],
)
>>>>>>> 4b50ac6c0980c0650ac2a24d84a901b36ce2298c
