package(default_visibility = ["//visibility:public"])

cc_library(
    name = "check_point_info",
    srcs = ["check_point_info.cpp"],
    hdrs = ["check_point_info.h"],
    deps = [
        "//common:comm",
        "//common/logging",
        "//config:resdb_config",
        "//crypto:signature_verifier",
        "//proto:checkpoint_info_cc_proto",
        "//proto:resdb_cc_proto",
    ],
)
