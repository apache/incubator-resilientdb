package(default_visibility = ["//visibility:private"])

load("@bazel_skylib//rules:common_settings.bzl", "bool_flag")

cc_binary(
    name = "kv_server_performance",
    srcs = ["kv_server_performance.cpp"],
    deps = [
        "//chain/storage:memory_db",
        "//executor/kv:kv_executor",
        "//platform/config:resdb_config_utils",
        "//platform/consensus/ordering/rcc/framework:consensus",
        "//service/utils:server_factory",
    ],
    linkopts = [
      "-l:libprofiler.a",
      "-pthread",
      "-L/usr/local/lib"
    ]
)

cc_binary(
    name = "test",
    srcs = ["test.cpp"],
    deps = [
    ],
    linkopts = [
      "-l:libprofiler.a",
      "-pthread",
      "-L/usr/local/lib"
    ],
    copts = [
      "-pg",
    ]
)

