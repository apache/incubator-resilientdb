. ./script/env.sh


KEY_FILE="config/key.conf"

. ${KEY_FILE}
iplist=(
172.31.29.163
172.31.22.39
172.31.16.32
172.31.27.98
172.31.23.49
172.31.18.178
172.31.25.109
172.31.17.109
172.31.18.116
172.31.25.117
172.31.28.50
172.31.24.115
172.31.17.251
172.31.17.252
172.31.27.122
172.31.26.123
172.31.25.31
#172.31.18.95
#172.31.23.208
#172.31.22.83
#172.31.20.214
#172.31.16.87
#172.31.26.116
#172.31.27.244
#172.31.18.245
#172.31.25.121
#172.31.29.121
#172.31.27.254
#172.31.21.255
#172.31.27.255
#172.31.28.96
#172.31.21.231
#172.31.21.236
#172.31.24.237
#172.31.30.240
#172.31.22.242
#172.31.18.49
#172.31.18.178
#172.31.17.179
#172.31.26.181
#172.31.31.54
#172.31.29.55
#172.31.30.188
#172.31.27.190
#172.31.17.162
#172.31.31.168
#172.31.20.169
#172.31.22.171
#172.31.28.172
#172.31.24.173
#172.31.28.45
#172.31.19.174
#172.31.31.215
#172.31.22.216

)

command1="sudo tc qdisc  add dev ens5 root netem delay 200ms"
command2="sudo tc qdisc  change dev ens5 root netem delay 200ms"
command3="tc qdisc show"

echo ${command1}

function run_cmd(){
  ip=$1
  cmd=$2
  ssh -i ${key} -n -o BatchMode=yes -o StrictHostKeyChecking=no ubuntu@${ip} "${cmd}"
  echo "ssh -i ${key} -n -o BatchMode=yes -o StrictHostKeyChecking=no ubuntu@${ip} \"${cmd}\""
}

for ip in ${iplist[@]};
do
  run_cmd ${ip} "${command1}" &
  #run_cmd ${ip} "${command2}" &
done

wait 

for ip in ${iplist[@]};
do
  run_cmd ${ip} "${command3}" 
done

wait 
