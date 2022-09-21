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
    name = "signature_verifier",
    srcs = ["signature_verifier.cpp"],
    hdrs = ["signature_verifier.h"],
    deps = [
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
