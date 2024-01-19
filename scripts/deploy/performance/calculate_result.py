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

import sys

total = 0
def read_tps(file):
    tps = []
    lat = []
    with open(file) as f:
        for l in f.readlines():
            s = l.split()
            for r in s:
                try:
                  if(r.split(':')[0] == 'txn'):
                      tps.append(int(r.split(':')[1]))
                except:
                  print("s:",s)
            if l.find("client latency") > 0:
                print("get lat:",s)
                lat.append(float(s[-1].split(':')[-1]))
    return tps, lat

def cal_tps(tps, tot):
    tps_sum = []
    tps_max = 0

    for v in tps:
        if v == 0:
            continue
        if v < 1000:
            continue
        tps_max = max(tps_max, v)
        tps_sum.append(v) 

    tps_sum.sort()
    tps_sum = tps_sum[tot:-tot]
    print("tsp:",tps_sum)
    print("max throughput:",tps_max)
    print("average throughput:",sum(tps_sum)/len(tps_sum))
    return tps_max, sum(tps_sum)/len(tps_sum)

def cal_lat(lat):
    lat_sum = []
    lat_max = 0
    for v in lat:
        if v <= 0:
            continue
        lat_max = max(lat_max, v)
        lat_sum.append(v) 

    print("max latency:",lat_max)
    print("average latency:",sum(lat_sum)/len(lat_sum))
    return lat_max, sum(lat_sum)/len(lat_sum)

if __name__ == '__main__':
    files = sys.argv[1:]
    print("calculate results, number of nodes:",len(files))


    tps = []
    lat = []
    total = len(files)
    for f in files:
        t, l=read_tps(f)
        tps += t
        lat += l

    max_tps, avg_tps = cal_tps(tps, len(files))
    max_lat, avg_lat = cal_lat(lat)

    print("max throughput:{} average throughput:{} "
    "max latency:{} average latency:{} "
    "replica num:{}".format(max_tps, avg_tps, max_lat, avg_lat, total))
