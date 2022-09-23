package(default_visibility = ["//visibility:public"])

cc_import(
  name="cryptopp_lib",
  hdrs = glob(
        ["**/*.h"],
    ),
  static_library="libcryptopp.a"
)

cc_library(
  name="cryptopp",
  includes = ["include"],
  deps = [":cryptopp_lib"],
)
