package(default_visibility = ["//visibility:public"])

cc_library(
    name = "resdb_kv_client",
    srcs = ["resdb_kv_client.cpp"],
    hdrs = ["resdb_kv_client.h"],
    deps = [
        "//client:resdb_user_client",
        "//proto:kv_server_cc_proto",
    ],
)

cc_binary(
    name = "kv_client_tools",
    srcs = ["kv_client_tools.cpp"],
    deps = [
        ":resdb_kv_client",
        "//config:resdb_config_utils",
    ],
)
