bazel build tools/key_generator_tools
path=$1
bazel-bin/tools/key_generator_tools $path
