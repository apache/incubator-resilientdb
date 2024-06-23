
# Prerequire 
python3.10

pip

```
sudo apt-get install python3.10-dev -y
sudo apt-get install python3-dev -y
sudo apt-get install python3-pip -y
```

# Install Protobuf
```
cd protobuf
./install_protobuf.sh
```

# Install Bazel
```
cd bazel
wget https://github.com/bazelbuild/bazelisk/releases/download/v1.8.1/bazelisk-darwin-amd64
chmod +x bazelisk-darwin-amd64
mkdir -p bin
mv bazelisk-darwin-amd64 bin/bazel

bin/bazel --version
echo "export PATH="$PATH:$PWD/bin"" >> ~/.bashrc
. ~/.bashrc
```
or
```
cd bazel
./install_bazel.sh
```


test bazel
```
bazel --version
```
