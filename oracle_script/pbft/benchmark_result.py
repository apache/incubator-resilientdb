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
        cmd="grep \'total request:\' /home/ubuntu/server{}.log | tail -1".format(idx)
        res=run_remote_cmd(svr_ip, cmd)
        res=res[0].decode('utf8')
        data = res.split()
        print("res:",data[-4])

if __name__ == '__main__':
    config_file=sys.argv[1]
    config = read_config(config_file)
    get_data(config)

