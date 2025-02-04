#!/bin/sh

sudo apt update
sudo apt install apt-transport-https curl gnupg -y
sudo apt-get install protobuf-compiler -y
sudo apt-get install rapidjson-dev -y

curl -fsSL https://bazel.build/bazel-release.pub.gpg | gpg --dearmor > bazel.gpg
sudo mv bazel.gpg /etc/apt/trusted.gpg.d/ 
echo "deb [arch=amd64] https://storage.googleapis.com/bazel-apt stable jdk1.8" | sudo tee /etc/apt/sources.list.d/bazel.list
curl https://bazel.build/bazel-release.pub.gpg | sudo apt-key add -
sudo apt update && sudo apt install bazel=5.0.0 -y
sudo apt install clang-format -y
rm $PWD/.git/hooks/pre-push
ln -s $PWD/hooks/pre-push $PWD/.git/hooks/pre-push

# install buildifier
bazel build @com_github_bazelbuild_buildtools//buildifier:buildifier

sudo apt-get install python3.10-dev -y
sudo apt-get install python3-dev -y
