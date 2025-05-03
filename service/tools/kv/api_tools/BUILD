package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "kv_service_tools",
    srcs = ["kv_service_tools.cpp"],
    deps = [
        "//common/proto:signature_info_cc_proto",
        "//interface/kv:kv_client",
        "//platform/config:resdb_config_utils",
    ],
)

cc_binary(
    name = "kv_client_txn_tools",
    srcs = ["kv_client_txn_tools.cpp"],
    deps = [
        "//interface/common:resdb_txn_accessor",
        "//platform/config:resdb_config_utils",
        "//proto/kv:kv_cc_proto",
    ],
)
