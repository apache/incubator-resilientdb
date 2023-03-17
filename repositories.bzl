load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")


def nexres_repositories():
  maybe(
        http_archive,
        name = "eEVM",
        strip_prefix = "eEVM-main",
        sha256 = "6321a6e355ddaa938a2deb5348e0d51cef8a44e0a998611578f97d014e450490",
        url = "https://github.com/microsoft/eEVM/archive/refs/heads/main.zip",
        build_file = "@com_resdb_nexres//third_party:eEVM.BUILD",
  )
  maybe(
        http_archive,
        name = "com_github_jupp0r_prometheus_cpp",
        strip_prefix = "prometheus-cpp-1.0.1",
        sha256 = "281b6d9a26da35375c9958954e03616d71ea28d57ec193b0e75c3e10ff3da55d",
        url = "https://github.com/jupp0r/prometheus-cpp/archive/refs/tags/v1.0.1.zip",
        build_file = "@com_resdb_nexres//third_party:prometheus.BUILD",
  )

  maybe(
      http_archive,
      name = "com_github_nelhage_rules_boost",
      strip_prefix = "rules_boost-96e9b631f104b43a53c21c87b01ac538ad6f3b48",
      url = "https://github.com/nelhage/rules_boost/archive/96e9b631f104b43a53c21c87b01ac538ad6f3b48.tar.gz",
  )





def _data_deps_extension_impl(ctx):
    nexres_repositories()

data_deps_ext = module_extension(
    implementation = _data_deps_extension_impl,
)
