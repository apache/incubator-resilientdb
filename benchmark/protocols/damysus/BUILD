package(default_visibility = ["//visibility:private"])

cc_binary(
    name = "kv_server_performance",
    srcs = ["kv_server_performance.cpp"],
    copts = ["-Iexternal/openenclave"],
    deps = [
        "//chain/storage:memory_db",
        "//executor/kv:kv_executor",
        "//platform/config:resdb_config_utils",
        "//platform/consensus/ordering/damysus/framework:consensus",
        "//service/utils:server_factory",
        "//enclave:headers",
    ],
)
