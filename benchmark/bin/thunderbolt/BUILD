package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "kv_server",
    srcs = ["kv_server.cpp"],
    deps = [
        "//platform/config:resdb_config_utils",
        "//platform/consensus/ordering/thunderbolt/framework:consensus",
        "//executor/kv:kv_executor",
        "//chain/storage:memory_db",
        "//service/utils:server_factory",
    ],
)
