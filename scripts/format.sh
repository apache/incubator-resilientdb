find . ! -path "./deps/*" -type f -regex ".*\.cpp\|.*\.h"  | xargs clang-format -i

bazel build @com_github_bazelbuild_buildtools//buildifier:buildifier
bazel-bin/external/com_github_bazelbuild_buildtools/buildifier/buildifier_/buildifier -r .
