<!--
  - Licensed to the Apache Software Foundation (ASF) under one
  - or more contributor license agreements.  See the NOTICE file
  - distributed with this work for additional information
  - regarding copyright ownership.  The ASF licenses this file
  - to you under the Apache License, Version 2.0 (the
  - "License"); you may not use this file except in compliance
  - with the License.  You may obtain a copy of the License at
  -
  -   http://www.apache.org/licenses/LICENSE-2.0
  -
  - Unless required by applicable law or agreed to in writing,
  - software distributed under the License is distributed on an
  - "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
  - KIND, either express or implied.  See the License for the
  - specific language governing permissions and limitations
  - under the License.
  -->

# Prerequire 
python3.10

pip

```
sudo apt update
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
echo "export PATH="$PATH:$PWD/bin"" >> ~/.bashrc
. ~/.bashrc
```


test bazel
```
bazel --version
```
