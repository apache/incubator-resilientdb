#export COPTS="--define enable_leveldb=True"

ssh_options_cloud='-o StrictHostKeyChecking=no -o LogLevel=ERROR -o UserKnownHostsFile=/dev/null -o ServerAliveInterval=60'


#!/bin/bash
eval "$(cat $1)"

echo "IP List:"
for ip in "${iplist[@]}"; do
    echo "$ip"
done

for ip in ${iplist[@]};
do
  ssh $ssh_options_cloud -p 2222 root@$ip 'echo "deb http://security.ubuntu.com/ubuntu focal-security main" | sudo tee -a /etc/apt/sources.list && sudo apt update && sudo apt install openssl=1.1.1f-1ubuntu2.22 --allow-downgrades -y' &
  # ssh $ssh_options_cloud root@$ip 'echo "deb http://security.ubuntu.com/ubuntu focal-security main" | sudo tee -a /etc/apt/sources.list && sudo apt update && sudo apt install openssl=1.1.1f-1ubuntu2.22 --allow-downgrades -y' &
done
wait

