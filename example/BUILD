package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "kv_server_tools",
    srcs = ["kv_server_tools.cpp"],
    deps = [
        "//config:resdb_config_utils",
        "//kv_client:resdb_kv_client",
        "//proto:signature_info_cc_proto",
    ],
)
