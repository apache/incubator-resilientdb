package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "kv_service_tools",
    srcs = ["kv_service_tools.cpp"],
    deps = [
        "//common/proto:signature_info_cc_proto",
        "//platform/config:resdb_config_utils",
        "//service/kv_service/interface:resdb_kv_client",
    ],
)

cc_binary(
    name = "kv_client_txn_tools",
    srcs = ["kv_client_txn_tools.cpp"],
    deps = [
        "//platform/config:resdb_config_utils",
        "//platform/interface:resdb_txn_accessor",
        "//service/kv_service/proto:kv_server_cc_proto",
    ],
)
