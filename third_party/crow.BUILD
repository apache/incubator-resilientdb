licenses(["notice"])

exports_files(["LICENSE"])

cc_library(
    name = "crow",
    srcs = glob([
	"include/**/*.h",
	"include/**/*.hpp",
    ]),
    hdrs = glob([
        "include/**/*.h",
        "include/**/*.hpp",
    ]),
    includes = ["include"],
    linkopts = ["-lpthread"],
    visibility = ["//visibility:public"],
    deps = ["//external:asio",],
) 
