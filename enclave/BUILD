package(default_visibility = ["//visibility:public"])

filegroup(
    name = "headers1",
    srcs = glob(["*.h"]),
)

cc_library(
    name = "headers",
    srcs = ["sgx_cpp_u.c"],
    hdrs = [
        ":headers1",
        "@openenclave//:headerfile"
    ],
    copts = ["-Iexternal/openenclave"],
    linkopts = [
        "-lcrypto",
        "-ldl",
        "-lpthread",
    ],
    deps = [
        "@openenclave//:headers",
        "@openenclave_host//:oehost_lib",
    ],
    visibility = ["//visibility:public"],
)

cc_binary(
    name = "oehost",
    srcs = [
        "host.cpp", 
    ],
    copts = ["-Iexternal/openenclave"],
    deps = [
        ":headers",
    ]
)
