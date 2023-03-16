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

def _data_deps_extension_impl(ctx):
    nexres_repositories()

data_deps_ext = module_extension(
    implementation = _data_deps_extension_impl,
)
