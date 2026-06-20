cc_binary(
    name = "kv_server_performance",
    srcs = ["kv_server_performance.cpp"],
    deps = [
        "//chain/storage:memory_db",
        "//executor/kv:kv_executor",
        "//platform/config:resdb_config_utils",
        "//platform/consensus/ordering/2PC:consensus",
        "//platform/networkstrate:service_network",
        "//proto/kv:kv_cc_proto",
    ],
)

cc_binary(
    name = "kv_service_tools",
    srcs = ["kv_service_tools.cpp"],
    deps = [
        "//common/proto:signature_info_cc_proto",
        "//interface/kv:kv_client",
        "//platform/config:resdb_config_utils",
    ],
)