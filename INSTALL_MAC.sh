!/bin/sh

sudo apt update
sudo apt install g++ -y
sudo apt install apt-transport-https curl gnupg -y
sudo apt install protobuf-compiler -y

wget https://github.com/bazelbuild/bazelisk/releases/download/v1.8.1/bazelisk-linux-arm64
chmod +x bazelisk-linux-arm64
sudo mv bazelisk-linux-arm64 /usr/local/bin/bazel

sudo apt install clang-format -y
rm -rf $PWD/.git/hooks/pre-push
ln -s $PWD/hooks/pre-push $PWD/.git/hooks/pre-push

bazel build @com_github_bazelbuild_buildtools//buildifier:buildifier

# for jemalloc
sudo apt-get install autoconf automake libtool -y

sudo apt install rapidjson-dev -y
