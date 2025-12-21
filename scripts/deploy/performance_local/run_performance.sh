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


./script/deploy_local.sh $1
. ./script/load_config.sh $1
. ./script/env.sh

home_path="./"
server_name=`echo "$server" | awk -F':' '{print $NF}'`
server_bin=${server_name}

bazel run //benchmark/protocols/pbft:kv_service_tools -- $PWD/config_out/client.config 

sleep 60

echo "benchmark done"
count=1
for ip in ${iplist[@]};
do
 echo "server bin:"${server_bin}
killall -9 ${server_bin}
((count++))
done

while [ $count -gt 0 ]; do
        wait $pids
        count=`expr $count - 1`
done

idx=1
echo "getting results"
for ip in ${iplist[@]};
do
  echo "cp ${home_path}/${server_bin}.log ./${ip}_${idx}_log"
  cp ${home_path}/resilientdb_app/${idx}/${server_bin}.log result_${ip}_${idx}_log
  ((idx++))
done

python3 performance/calculate_result.py `ls result_*_log` > results.log

rm -rf result_*_log
echo "save result to results.log"
cat results.log
