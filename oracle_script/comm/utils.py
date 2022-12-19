import subprocess
import os
from oracle_script.comm.comm_config import *

def run_cmd(cmd):
    print(cmd)
    p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output=p.stdout.readlines()
    p.wait()
    return output

def run_cmd_no_wait(cmd):
    print(cmd)
    subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

def run_cmd_with_resp(cmd):
    res=run_cmd(cmd)
    if(len(res)>0):
        print (res)

def run_remote_cmd(ip, cmd):
    return run_cmd("ssh -i {} -n -o BatchMode=yes -o StrictHostKeyChecking=no ubuntu@{} \" {} \"" .format(KEY, ip, cmd))

def run_remote_cmd_list(cmds):
    p_list=[]
    for ip,cmd in cmds:
        script=("ssh -i {} -n -o BatchMode=yes -o StrictHostKeyChecking=no ubuntu@{} \" {} \" &" .format(KEY, ip, cmd))
        print(script)
        p = subprocess.Popen(script, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        p_list.append(p)
    for p in p_list:
        output=p.stdout.readlines()
        for o in output:
            print(o.decode('utf8'))
        p.wait()

def run_remote_cmd_list_raw(cmds):
    p_list=[]
    for cmd in cmds:
        cmd = "{} &".format(cmd)
        print(cmd)
        p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        p_list.append(p)
    for p in p_list:
        output=p.stdout.readlines()
        for o in output:
            print(o.decode('utf8'))
        p.wait()

def get_ips(ip_file):
    iplist=[]
    with open(ip_file) as f:
        for item in f.readlines():
            item = item.strip()
            if(len(item) == 0): 
                    continue
            iplist.append(item.split())
    return iplist 

def read_config(config_file):
    config = {}
    with open(config_file) as f:
        for item in f.readlines():
            item = item.strip().split("=")
            if(len(item) !=2):
                continue
            config[item[0]]=item[1].strip()
    return config


def get_remote_file_name(file_path):
    return "/home/ubuntu/"+os.path.basename(file_path)
