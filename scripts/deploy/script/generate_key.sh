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
bazel_path=$1; shift
output_path=$1; shift
key_num=$1

echo "generate key in:"${output_path}
echo "key num:"$key_num

bazel build //tools:key_generator_tools
rm -rf ${output_path}
mkdir -p ${output_path}

for idx in `seq 1 ${key_num}`;
do
  echo `${bazel_path}/bazel-bin/tools/key_generator_tools "${output_path}/node_${idx}" "AES"`
done
