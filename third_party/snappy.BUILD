package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # BSD 3-clause

filegroup(
    name = "license",
    srcs = ["COPYING"],
)

genrule(
    name = "snappy_stubs_public_h",
    srcs = [
        "snappy-stubs-public.h.in",
    ],
    outs = [
        "snappy-stubs-public.h",
    ],
    cmd =
        "sed 's/@ac_cv_have_stdint_h@/1/g' $(<) | " +
        "sed 's/@ac_cv_have_stddef_h@/1/g' | " +
        "sed 's/@ac_cv_have_sys_uio_h@/1/g' | " +
        "sed 's/@SNAPPY_MAJOR@/1/g' | " +
        "sed 's/@SNAPPY_MINOR@/1/g' | " +
        "sed s/'$${HAVE_SYS_UIO_H_01}'/1/g | " +
        "sed s/'$${HAVE_STDINT_H_01}'/1/g  | " +
        "sed s/'$${HAVE_STDDEF_H_01}'/1/g  | " +
        "sed 's/@SNAPPY_PATCHLEVEL@/3/g' > $(@)",
)

cc_library(
    name = "snappy",
    srcs = [
        "snappy.cc",
        "snappy-c.cc",
        "snappy-sinksource.cc",
        "snappy-stubs-internal.cc",
    ],
    hdrs = [
        "snappy.h",
        "snappy-c.h",
        "snappy-internal.h",
        "snappy-sinksource.h",
        "snappy-stubs-internal.h",
        ":snappy-stubs-public.h",
    ],
    copts = [
        "-Wno-non-virtual-dtor",
        "-Wno-unused-variable",
        "-Wno-implicit-fallthrough",
        "-Wno-unused-function",
    ],
    includes = ["."],
)
