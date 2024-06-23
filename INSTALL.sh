#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#

#apt update
#apt install apt-transport-https curl gnupg -y
apt-get install protobuf-compiler -y
apt-get install rapidjson-dev -y

bazel --version
ret=$?

if [[ $ret != "0" ]]; then
echo "please ensure Bazel 6.0has been installed"
fi

# install buildifier
bazel build @com_github_bazelbuild_buildtools//buildifier:buildifier

apt-get install python3.10-dev -y
apt-get install python3-dev -y
