load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "rules_foreign_cc",
    sha256 = "69023642d5781c68911beda769f91fcbc8ca48711db935a75da7f6536b65047f",
    strip_prefix = "rules_foreign_cc-0.6.0",
    url = "https://github.com/bazelbuild/rules_foreign_cc/archive/0.6.0.tar.gz",
)

load("@rules_foreign_cc//foreign_cc:repositories.bzl", "rules_foreign_cc_dependencies")

rules_foreign_cc_dependencies()

http_archive(
    name = "rules_proto",
    sha256 = "66bfdf8782796239d3875d37e7de19b1d94301e8972b3cbd2446b332429b4df1",
    strip_prefix = "rules_proto-4.0.0",
    urls = [
        "https://mirror.bazel.build/github.com/bazelbuild/rules_proto/archive/refs/tags/4.0.0.tar.gz",
        "https://github.com/bazelbuild/rules_proto/archive/refs/tags/4.0.0.tar.gz",
    ],
)

load("@rules_proto//proto:repositories.bzl", "rules_proto_dependencies", "rules_proto_toolchains")

rules_proto_dependencies()

rules_proto_toolchains()

http_archive(
    name = "rules_python",
    sha256 = "ffc7b877c95413c82bfd5482c017edcf759a6250d8b24e82f41f3c8b8d9e287e",
    strip_prefix = "rules_python-0.19.0",
    url = "https://github.com/bazelbuild/rules_python/releases/download/0.19.0/rules_python-0.19.0.tar.gz",
)

load("@rules_python//python:pip.bzl", "pip_install")

http_archive(
    name = "rules_proto_grpc",
    sha256 = "fb7fc7a3c19a92b2f15ed7c4ffb2983e956625c1436f57a3430b897ba9864059",
    strip_prefix = "rules_proto_grpc-4.3.0",
    urls = ["https://github.com/rules-proto-grpc/rules_proto_grpc/archive/4.3.0.tar.gz"],
)

load("@rules_proto_grpc//:repositories.bzl", "rules_proto_grpc_repos", "rules_proto_grpc_toolchains")

rules_proto_grpc_toolchains()

rules_proto_grpc_repos()

load("@rules_proto//proto:repositories.bzl", "rules_proto_dependencies", "rules_proto_toolchains")

rules_proto_dependencies()

rules_proto_toolchains()

bind(
    name = "gtest",
    actual = "@com_google_googletest//:gtest",
)

http_archive(
    name = "com_google_googletest",
    sha256 = "ffa17fbc5953900994e2deec164bb8949879ea09b411e07f215bfbb1f87f4632",
    strip_prefix = "googletest-1.13.0",
    urls = ["https://github.com/google/googletest/archive/refs/tags/v1.13.0.zip"],
)

http_archive(
    name = "com_github_gflags_gflags",
    sha256 = "34af2f15cf7367513b352bdcd2493ab14ce43692d2dcd9dfc499492966c64dcf",
    strip_prefix = "gflags-2.2.2",
    urls = ["https://github.com/gflags/gflags/archive/v2.2.2.tar.gz"],
)

bind(
    name = "glog",
    actual = "@com_github_google_glog//:glog",
)

http_archive(
    name = "com_github_google_glog",
    sha256 = "21bc744fb7f2fa701ee8db339ded7dce4f975d0d55837a97be7d46e8382dea5a",
    strip_prefix = "glog-0.5.0",
    urls = ["https://github.com/google/glog/archive/v0.5.0.zip"],
)

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

git_repository(
    name = "com_google_protobuf",
    remote = "https://github.com/protocolbuffers/protobuf",
    tag = "v3.10.0",
)

load("@com_google_protobuf//:protobuf_deps.bzl", "protobuf_deps")

protobuf_deps()

http_archive(
    name = "com_google_absl",
    strip_prefix = "abseil-cpp-20211102.0",
    urls = ["https://github.com/abseil/abseil-cpp/archive/refs/tags/20211102.0.zip"],
)

http_archive(
    name = "com_github_nelhage_rules_boost",
    strip_prefix = "rules_boost-96e9b631f104b43a53c21c87b01ac538ad6f3b48",

    # Replace the commit hash in both places (below) with the latest, rather than using the stale one here.
    # Even better, set up Renovate and let it do the work for you (see "Suggestion: Updates" in the README).
    url = "https://github.com/nelhage/rules_boost/archive/96e9b631f104b43a53c21c87b01ac538ad6f3b48.tar.gz",
    # When you first run this tool, it'll recommend a sha256 hash to put here with a message like: "DEBUG: Rule 'com_github_nelhage_rules_boost' indicated that a canonical reproducible form can be obtained by modifying arguments sha256 = ..."
)

load("@com_github_nelhage_rules_boost//:boost/boost.bzl", "boost_deps")

boost_deps()

http_archive(
    name = "net_zlib_zlib",
    build_file = "@com_resdb_nexres//third_party:z.BUILD",
    sha256 = "91844808532e5ce316b3c010929493c0244f3d37593afd6de04f71821d5136d9",
    strip_prefix = "zlib-1.2.12",
    urls = [
        "https://zlib.net/zlib-1.2.12.tar.gz",
        "https://storage.googleapis.com/bazel-mirror/zlib.net/zlib-1.2.12.tar.gz",
    ],
)

http_archive(
    name = "com_crowcpp_crow",
    build_file = "//third_party:crow.BUILD",
    sha256 = "f95128a8976fae6f2922823e07da59edae277a460776572a556a4b663ff5ee4b",
    strip_prefix = "Crow-1.0-5",
    url = "https://github.com/CrowCpp/Crow/archive/refs/tags/v1.0+5.zip",
)

bind(
    name = "asio",
    actual = "@com_chriskohlhoff_asio//:asio",
)

http_archive(
    name = "com_chriskohlhoff_asio",
    build_file = "//third_party:asio.BUILD",
    sha256 = "babcdfd2c744905a73d20de211b51367bda0d5200f11d654c4314b909d8c963c",
    strip_prefix = "asio-asio-1-26-0",
    url = "https://github.com/chriskohlhoff/asio/archive/refs/tags/asio-1-26-0.zip",
)

http_archive(
    name = "pybind11_bazel",
    strip_prefix = "pybind11_bazel-2.11.1.bzl.1",
    urls = ["https://github.com/pybind/pybind11_bazel/archive/refs/tags/v2.11.1.bzl.1.zip"]
)

http_archive(
    name = "pybind11",
    build_file = "@pybind11_bazel//:pybind11.BUILD",
    sha256 = "8ff2fff22df038f5cd02cea8af56622bc67f5b64534f1b83b9f133b8366acff2",
    strip_prefix = "pybind11-2.6.2",
    urls = ["https://github.com/pybind/pybind11/archive/v2.6.2.tar.gz"],
)

load("@pybind11_bazel//:python_configure.bzl", "python_configure")

python_configure(
    name = "local_config_python",
    python_version = "3",
)

http_archive(
    name = "rapidjson",
    build_file = "//third_party:rapidjson.BUILD",
    sha256 = "8e00c38829d6785a2dfb951bb87c6974fa07dfe488aa5b25deec4b8bc0f6a3ab",
    strip_prefix = "rapidjson-1.1.0",
    url = "https://github.com/Tencent/rapidjson/archive/refs/tags/v1.1.0.zip",
)

http_archive(
    name = "civetweb",
    build_file = "@com_resdb_nexres//third_party:civetweb.BUILD",
    sha256 = "88574f0cffd6047e22fafa3bdc748dd878a4928409d4f880332e2b0f262b9f62",
    strip_prefix = "civetweb-1.15",
    url = "https://github.com/civetweb/civetweb/archive/refs/tags/v1.15.zip",
)

#git_repository(
#    name = "com_resdb_nexres",
#    branch = "master",
#    remote = "https://github.com/resilientdb/resilientdb.git",
#)
git_repository(
    name = "com_resdb_nexres",
    branch = "master",
    remote = "https://github.com/apache/incubator-resilientdb.git",
)

http_archive(
    name = "io_bazel_rules_go",
    sha256 = "d6b2513456fe2229811da7eb67a444be7785f5323c6708b38d851d2b51e54d83",
    urls = [
        "https://mirror.bazel.build/github.com/bazelbuild/rules_go/releases/download/v0.30.0/rules_go-v0.30.0.zip",
        "https://github.com/bazelbuild/rules_go/releases/download/v0.30.0/rules_go-v0.30.0.zip",
    ],
)

load("@io_bazel_rules_go//go:deps.bzl", "go_rules_dependencies")

go_rules_dependencies()

load("@io_bazel_rules_go//go:deps.bzl", "go_register_toolchains")

go_register_toolchains(version = "1.19.5")

http_archive(
    name = "com_github_bazelbuild_buildtools",
    sha256 = "518b2ce90b1f8ad7c9a319ca84fd7de9a0979dd91e6d21648906ea68faa4f37a",
    strip_prefix = "buildtools-5.0.1",
    urls = [
        "https://github.com/bazelbuild/buildtools/archive/refs/tags/5.0.1.zip",
    ],
)

load("@com_resdb_nexres//:repositories.bzl", "nexres_repositories")

nexres_repositories()