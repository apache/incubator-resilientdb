licenses(["notice"])

exports_files(["LICENSE"])

cc_library(
    name = "pistache",
    srcs = glob(["src/**/*.cc"]),
    hdrs = glob(["include/**/*.h"]),
    includes = ["include"],
    linkopts = ["-lpthread"],
    visibility = ["//visibility:public"],
    deps = ["//external:date",],
)