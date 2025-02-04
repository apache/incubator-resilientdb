package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "kv_server",
    srcs = ["kv_server.cpp"],
    deps = [
        "//platform/config:resdb_config_utils",
        "//platform/consensus/ordering/tusk:consensus_service_tusk",
        "//service/kv_service/executor:kv_server_executor",
        "//service/utils:server_factory",
    ],
)
