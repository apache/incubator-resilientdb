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
    # tps_sum = tps_sum[1:-1]
    # tps_sum = tps_sum[tot:-tot]
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

def read_breakdown(file):
    queueing=[] # queuing latency
    receive_proposal=[] # execute_prepare latency
    verify_proposal=[] # verify latency
    
    commit=[] # commit latency

    commit_runtime=[] # commit_running
    execute_queueing=[] # execute_queuing
    execute=[] # execute latency  

    commit_round=[] # how many round needed for commit. commit_round latency

    with open(file) as f:
        for line in f:
            if "queuing latency" in line:
                lat=parse_latency(line, "queuing latency")
                if (lat != float('nan') and lat>0.0):
                    queueing.append(lat)
            if "execute_prepare latency" in line:
                lat=parse_latency(line, "execute_prepare latency")
                if (lat != float('nan') and lat>0.0):
                    receive_proposal.append(lat)
            if "verify latency" in line:
                lat=parse_latency(line, "verify latency")
                if (lat != float('nan') and lat>0.0):
                    verify_proposal.append(lat)
            if "commit latency" in line:
                lat=parse_latency(line, "commit latency")
                if (lat != float('nan') and lat>0.0):
                    commit.append(lat)            
            if "commit_running latency" in line:
                lat=parse_latency(line, "commit_running latency")
                if (lat != float('nan') and lat>0.0):
                    commit_runtime.append(lat)
            if "execute_queuing latency" in line:
                lat=parse_latency(line, "execute_queuing latency")
                if (lat != float('nan') and lat>0.0):
                    execute_queueing.append(lat)
            if "execute latency" in line:
                lat=parse_latency(line, "execute latency")
                if (lat != float('nan') and lat>0.0):
                    execute.append(lat)
            if "commit_round latency" in line:
                lat=parse_latency(line, "commit_round latency")
                if (lat != float('nan') and lat>0.0):
                    commit_round.append(lat)

    return {
        "queueing": queueing,
        "receive_proposal": receive_proposal,
        "verify_proposal": verify_proposal,
        "commit": commit,
        "commit_runtime": commit_runtime,
        "execute_queueing": execute_queueing,
        "execute": execute,
        "commit_round": commit_round,
    }

def parse_latency(line, key):
    """Extract latency value from a log line based on a key."""
    try:
        return float(line.split(f"{key} :")[1].split()[0])
    except (IndexError, ValueError):
        return None

def analyze_breakdown(data):
    """Analyze breakdown data: calculate average and max latencies."""
    for key, values in data.items():
        if values:
            avg = sum(values) / len(values)
            max_val = max(values)
            min_val = min(values)
            print(f"{key}: min = {min_val:.6f}, max = {max_val:.6f}, avg = {avg:.6f}")
        else:
            print(f"{key}: No data available.")



if __name__ == '__main__':
    files = sys.argv[1:]
    print("calculate results, number of nodes:",len(files))


    tps = []
    lat = []
    breakdown_data = {
        "queueing": [],
        "receive_proposal": [],
        "verify_proposal": [],
        "commit_runtime": [],
        "commit": [],
        "execute_queueing": [],
        "execute": [],
        "commit_round": [],
    }
    total = len(files)
    for f in files:
        t, l=read_tps(f)
        tps += t
        lat += l

        # Read breakdown data
        breakdown = read_breakdown(f)
        for key in breakdown_data:
            breakdown_data[key].extend(breakdown[key])


    # Analyze breakdown data
    print("\nBreakdown Analysis:")
    analyze_breakdown(breakdown_data)
    
    print()
    max_tps, avg_tps = cal_tps(tps, len(files))
    max_lat, avg_lat = cal_lat(lat)

    print("max throughput:{} average throughput:{} "
    "max latency:{} average latency:{} "
    "replica num:{}".format(max_tps, avg_tps, max_lat, avg_lat, total))
