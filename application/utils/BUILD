package(default_visibility = ["//visibility:public"])

cc_library(
    name = "server_factory",
    srcs = ["server_factory.cpp"],
    hdrs = ["server_factory.h"],
    deps = [
        "//config:resdb_config_utils",
        "//execution:custom_query",
        "//ordering/pbft:consensus_service_pbft",
        "//server:resdb_server",
    ],
)
