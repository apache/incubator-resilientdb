package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "pybind_kv.so",
    srcs = ["pybind_kv_service.cpp"],
    linkshared = 1,
    linkstatic = 1,
    deps = [
        "@//common/proto:signature_info_cc_proto",
        "@//interface/kv:kv_client",
        "@//platform/config:resdb_config_utils",
        "@pybind11",
    ],
)

py_library(
    name = "pybind_kv_so",
    data = [":pybind_kv.so"],
    imports = ["."],
)
