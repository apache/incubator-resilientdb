package(default_visibility = ["//visibility:public"])

cc_library(
    name = "key_generator",
    srcs = ["key_generator.cpp"],
    hdrs = ["key_generator.h"],
    deps = [
        "//:cryptopp_lib",
        "//common:comm",
        "//proto:signature_info_cc_proto",
    ],
)

cc_library(
    name = "hash",
    srcs = ["hash.cpp"],
    hdrs = ["hash.h"],
    deps = [
        "//:cryptopp_lib",
        "//common:comm",
    ],
)

cc_test(
    name = "hash_test",
    srcs = ["hash_test.cpp"],
    deps = [
        ":hash",
        "//common/test:test_main",
    ],
)

cc_library(
    name = "signature_utils",
    srcs = ["signature_utils.cpp"],
    hdrs = ["signature_utils.h"],
    deps = [
        "//:cryptopp_lib",
        "//common:comm",
        "//proto:signature_info_cc_proto",
    ],
)

cc_library(
    name = "signature_verifier",
    srcs = ["signature_verifier.cpp"],
    hdrs = ["signature_verifier.h"],
    deps = [
        ":signature_utils",
        "//:cryptopp_lib",
        "//common:comm",
        "//proto:signature_info_cc_proto",
    ],
)

cc_library(
    name = "mock_signature_verifier",
    testonly = True,
    hdrs = ["mock_signature_verifier.h"],
    deps = [
        ":signature_verifier",
    ],
)

cc_test(
    name = "signature_verifier_test",
    srcs = ["signature_verifier_test.cpp"],
    deps = [
        ":key_generator",
        ":signature_verifier",
        "//common/test:test_main",
    ],
)
