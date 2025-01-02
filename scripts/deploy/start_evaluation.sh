#!/bin/bash

ssh_options_cloud='-o StrictHostKeyChecking=no -o LogLevel=ERROR -o UserKnownHostsFile=/dev/null -o ServerAliveInterval=60'


# cat <<EOF > config/kv_performance_server.conf

# iplist=(
# 172.17.0.2
# 172.18.218.170
# )

# client_num=1
# EOF

source config/kv_performance_server.conf

echo "IP List:"
for ip in "${iplist[@]}"; do
    echo "$ip"
done

# for ip in ${iplist[@]};
# do
#   ssh $ssh_options_cloud root@$ip 'wget http://nz2.archive.ubuntu.com/ubuntu/pool/main/o/openssl/libssl1.1_1.1.1f-1ubuntu2.23_amd64.deb && sudo dpkg -i libssl1.1_1.1.1f-1ubuntu2.23_amd64.deb' &
# #   ssh $ssh_options_cloud root@$ip "sudo ip addr add $ip/32 dev eth0" &
# #   echo "sudo ip addr add $ip/32 dev eth0"
# #   ssh $ssh_options_cloud root@$ip 'echo "deb http://security.ubuntu.com/ubuntu focal-security main" | sudo tee -a /etc/apt/sources.list && sudo apt update && sudo apt install openssl=1.1.1f-1ubuntu2.22 --allow-downgrades -y' &
# done
# wait

mode=$1
if [[ "$mode" != "sgx" && "$mode" != "simulate" ]]; then
  echo "Invalid mode. Please specify 'sgx' or 'simulate'."
  exit 1
fi

deploy_file='./script/deploy.sh'
if [[ "$mode" == "sgx" ]]; then
  if grep -q '${enclave} --simulate' "$deploy_file"; then
    sed -i 's/${enclave} --simulate/${enclave}/g' "$deploy_file"
  fi
else
  if ! grep -q '${enclave} --simulate' "$deploy_file"; then
    sed -i 's/${enclave}/${enclave} --simulate/g' "$deploy_file"
  fi
fi

./performance/fides_performance.sh config/kv_performance_server.conf
# ./performance/run_performance.sh config/kv_performance_server.conf



# ldconfig /usr/local/lib64/ ->
# libssl.so.3: cannot open shared object file: No such file or directory 
# apt-get install linux-headers-$(uname -r)
