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
set +e

CURRENT_PATH=$PWD

i=0
while [ ! -f "WORKSPACE" ]
do
cd ..
((i++))
if [ "$PWD" = "/home" ]; then
  break
fi
done

BAZEL_WORKSPACE_PATH=$PWD
if [ "$PWD" = "/home" ]; then
echo "bazel path not found"
BAZEL_WORKSPACE_PATH=$CURRENT_PATH
fi

export BAZEL_WORKSPACE_PATH=$PWD

echo "use bazel path:"$BAZEL_WORKSPACE_PATH

# go back to the current dir
cd $CURRENT_PATH
