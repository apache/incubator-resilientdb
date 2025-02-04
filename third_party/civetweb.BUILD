package(default_visibility = ["//visibility:public"])

cc_library(
    name = "civetweb",
    srcs = [
        "src/CivetServer.cpp",
        "src/civetweb.c",
        "src/response.inl",
    ],
    hdrs = [
        "include/CivetServer.h",
        "include/civetweb.h",
    ],
    copts = [
        "-DUSE_IPV6",
        "-DNDEBUG",
        "-DNO_CGI",
        "-DNO_CACHING",
        "-DNO_SSL",
        "-DNO_FILES",
    ],
    textual_hdrs = [
        "src/md5.inl",
        "src/handle_form.inl",
    ],
    includes = [
        "include",
    ],
    visibility = ["//visibility:public"],
)
