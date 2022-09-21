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

cc_library(
    name = "resdb_kv_performance_client",
    srcs = ["resdb_kv_performance_client.cpp"],
    hdrs = ["resdb_kv_performance_client.h"],
    deps = [
        ":resdb_kv_client",
    ],
)

cc_binary(
    name = "kv_performance_client_main",
    srcs = ["kv_performance_client_main.cpp"],
    deps = [
        ":resdb_kv_performance_client",
        "//config:resdb_config_utils",
        "//proto:signature_info_cc_proto",
    ],
)
