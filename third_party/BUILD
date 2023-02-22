package(default_visibility = ["//visibility:public"])

load("@rules_foreign_cc//foreign_cc:defs.bzl", "configure_make", "make")

cc_library(
    name = "rocksdb",
    deps = [
        "@com_github_facebook_rocksdb//:rocksdb",
    ],
)

cc_library(
    name = "prometheus",
    deps = [
        "@com_github_jupp0r_prometheus_cpp//:prometheus",
    ],
)

configure_make(
    name = "jemalloc",
    autogen = True,
    configure_in_place = True,
    configure_options = [
        #    "--enable-prof",
    ],
    copts = [
        "-Wsign-compare",
    ],
    lib_source = "@com_jemalloc//:all_srcs",
    out_static_libs = ["libjemalloc.a"],
    targets = [
        "build_lib_static",
        "install_lib",
        "install_include",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "snappy",
    deps = [
        "@com_google_snappy//:snappy",
    ],
)

make(
    name = "zstd",
    lib_source = "@com_facebook_zstd//:all_srcs",
    out_static_libs = ["libzstd.a"],
)

cc_library(
    name = "leveldb",
    deps = [
        "@com_google_leveldb//:leveldb",
    ],
)

cc_library(
    name = "date",
    deps = [
        "@com_howardhinnant_date//:date",
    ],
)

cc_library(
    name = "crow",
    deps = [
        "@com_crowcpp_crow//:crow",
    ],
)

cc_library(
    name = "asio",
    deps = [
        "@com_chriskohlhoff_asio//:asio",
    ],
)

load("@rules_foreign_cc//foreign_cc:defs.bzl", "cmake")

cc_library(
    name = "evm_lib",
    deps = [
        "@eEVM",
    ],
)

cc_library(
    name = "rapidjson",
    deps = [
        "@rapidjson",
    ],
)
