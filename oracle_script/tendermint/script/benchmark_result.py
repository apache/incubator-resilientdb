#!/bin/sh

import sys

from oracle_script.comm.utils import *
from oracle_script.comm.comm_config import *


def get_data(config):
    iplist=get_ips(config["svr_ip_file"])
    count = {}
    avg_time = [] #time / per request
    req_time = [] #request/s 
    tot = 0
    for (idx,svr_ip) in iplist:
        cmd="grep \'txn:\' /home/ubuntu/server{}.log | grep -v 'txn:0' | tail -10".format(idx)
        res=run_remote_cmd(svr_ip, cmd)
        res=res[0].decode('utf8')
        data = res.split()
        print("data:{}".format(data))
    #print("tot:{}, time/request:{}, req/s:{}, count:{}".format(tot, sum(avg_time)/len(avg_time),sum(req_time)/len(req_time), count))
    #print("count:{} avg time:{} tot:{}".format(count, avg_time/tot, tot))

if __name__ == '__main__':
    config_file=sys.argv[1]
    config = read_config(config_file)
    get_data(config)

