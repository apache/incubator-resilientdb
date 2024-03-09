package(default_visibility = ["//visibility:public"])
cc_binary(
    name = "pybind_kv.so",
    srcs = ["pybind_kv_service.cpp"],
    linkshared =1,
    linkstatic = 1,
    deps = [
        "@com_resdb_nexres//common/proto:signature_info_cc_proto",
        "@com_resdb_nexres//interface/kv:kv_client",
        "@com_resdb_nexres//platform/config:resdb_config_utils",
        "@pybind11//:pybind11",
    ],
)
py_library(
    name = "pybind_kv_so",
    data = [":pybind_kv.so"],
    imports = ["."],
)

