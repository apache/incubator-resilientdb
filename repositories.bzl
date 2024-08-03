load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")

all_content = """filegroup(name = "all_srcs", srcs = glob(["**"]), visibility = ["//visibility:public"])"""

def nexres_repositories():
    maybe(
        http_archive,
        name = "eEVM",
        strip_prefix = "eEVM-118a9355d023748318a318bc07fc79063f015a94",
        sha256 = "e86568aec425405fd8a48bbe487edeae4c0641be23b19411288e3b736018e1b6",
        url = "https://github.com/microsoft/eEVM/archive/118a9355d023748318a318bc07fc79063f015a94.tar.gz",
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
    maybe(
        http_archive,
        name = "cryptopp",
        build_file_content = all_content,
        sha256 = "6055ab314ff4daae9490ddfb3fbcf107bc94a556401ed42f756fa5f7cd8c6510",
        strip_prefix = "cryptopp-CRYPTOPP_8_7_0",
        urls = ["https://github.com/weidai11/cryptopp/archive/refs/tags/CRYPTOPP_8_7_0.zip"],
    )
    maybe(
        http_archive,
        name = "com_google_absl",
        strip_prefix = "abseil-cpp-20211102.0",
        urls = ["https://github.com/abseil/abseil-cpp/archive/refs/tags/20211102.0.zip"],
    )
    maybe(
        http_archive,
        name = "com_google_googletest",
        sha256 = "ffa17fbc5953900994e2deec164bb8949879ea09b411e07f215bfbb1f87f4632",
        strip_prefix = "googletest-1.13.0",
        urls = ["https://github.com/google/googletest/archive/refs/tags/v1.13.0.zip"],
    )
    maybe(
        http_archive,
        name = "com_github_gflags_gflags",
        sha256 = "34af2f15cf7367513b352bdcd2493ab14ce43692d2dcd9dfc499492966c64dcf",
        strip_prefix = "gflags-2.2.2",
        urls = ["https://github.com/gflags/gflags/archive/v2.2.2.tar.gz"],
    )
    maybe(
        http_archive,
        name = "rules_proto_grpc",
        sha256 = "fb7fc7a3c19a92b2f15ed7c4ffb2983e956625c1436f57a3430b897ba9864059",
        strip_prefix = "rules_proto_grpc-4.3.0",
        urls = ["https://github.com/rules-proto-grpc/rules_proto_grpc/archive/4.3.0.tar.gz"],
    )
    maybe(
        http_archive,
        name = "net_zlib_zlib",
        build_file = "@com_resdb_nexres//third_party:z.BUILD",
        sha256 = "91844808532e5ce316b3c010929493c0244f3d37593afd6de04f71821d5136d9",
        strip_prefix = "zlib-1.2.12",
        urls = [
            "https://zlib.net/zlib-1.2.12.tar.gz",
            "https://storage.googleapis.com/bazel-mirror/zlib.net/zlib-1.2.12.tar.gz",
        ],
    )
    maybe(
        http_archive,
        name = "civetweb",
        build_file = "@com_resdb_nexres//third_party:civetweb.BUILD",
        sha256 = "88574f0cffd6047e22fafa3bdc748dd878a4928409d4f880332e2b0f262b9f62",
        strip_prefix = "civetweb-1.15",
        url = "https://github.com/civetweb/civetweb/archive/refs/tags/v1.15.zip",
    )

def _data_deps_extension_impl(ctx):
    nexres_repositories()

data_deps_ext = module_extension(
    implementation = _data_deps_extension_impl,
)
