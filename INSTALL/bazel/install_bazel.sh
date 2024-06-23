wget wget https://github.com/bazelbuild/bazelisk/releases/download/v1.8.1/bazelisk-linux-amd64
chmod +x bazelisk-linux-amd64
mkdir -p bin
mv bazelisk-linux-amd64 bin/bazel

bin/bazel --version
