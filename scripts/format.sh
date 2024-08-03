find . ! -path "./deps/*" -type f -regex ".*\.cpp\|.*\.h"  | xargs clang-format -i
bazel-bin/external/com_github_bazelbuild_buildtools/buildifier/buildifier_/buildifier -r .
