
iplist=`cat iplist.txt`
. benchmark.conf
key=~/.ssh/junchao.pem
cur_path=$PWD

count=1
for ip in ${iplist[@]};
do
	echo ${ip}
	ssh -i ${key} -n -o BatchMode=yes -o StrictHostKeyChecking=no ubuntu@${ip} " killall -9 kv_server_performance; " &
	((count++))
done

while [ $count -gt 0 ]; do
wait $pids
count=`expr $count - 1`
done

echo "================ kill done ======"

count=1
for ip in ${iplist[@]};
do
	ssh -i ${key} -n -o BatchMode=yes -o StrictHostKeyChecking=no ubuntu@${ip} " rm -rf /home/ubuntu/kv_server_performance; rm server*.log; rm -rf server.config; rm -rf cert; mkdir -p pbft_cert/; " &
	((count++))
done

while [ $count -gt 0 ]; do
wait $pids
count=`expr $count - 1`
done

set -x

count=1
idx=1
for ip in ${iplist[@]};
do
scp -i ${key} ${svr_bin_bazel_path} ubuntu@${ip}:/home/ubuntu &
scp -i ${key} ${cur_path}/server.config* ubuntu@${ip}:/home/ubuntu &
scp -i ${key} ${cur_path}/cert/node_${idx}.key.pri ubuntu@${ip}:/home/ubuntu/pbft_cert/ &
scp -i ${key} ${cur_path}/cert/cert_${idx}.cert ubuntu@${ip}:/home/ubuntu/pbft_cert/ &
	((count++))
	((count++))
	((count++))
	((count++))
	((idx++))
done

while [ $count -gt 0 ]; do
wait $pids
count=`expr $count - 1`
done

echo "================ rm done ======"

total_ip=`expr $idx - 1`
total_ip=`expr $total_ip - $region`
per_region=`expr $total_ip / $region`


idx=1
client_idx=`expr $total_ip + 1`
echo "client idx:"$client_idx," region:"$region" tot:"$total_ip
for ((r=1; r<=region; r++))
do
. region_ip${r}.txt
count=1
num=${#iplist[@]}
for ip in ${iplist[@]};
do
	if [ $count -eq $num ]; then
		ssh -i ${key} -n -o BatchMode=yes -o StrictHostKeyChecking=no ubuntu@${ip} " nohup /home/ubuntu/kv_server_performance /home/ubuntu/server.config_${r} pbft_cert//node_${client_idx}.key.pri pbft_cert//cert_${client_idx}.cert > server${client_idx}.log 2>&1 & " &
		((client_idx++))
	else
		ssh -i ${key} -n -o BatchMode=yes -o StrictHostKeyChecking=no ubuntu@${ip} " nohup /home/ubuntu/kv_server_performance /home/ubuntu/server.config_${r} pbft_cert//node_${idx}.key.pri pbft_cert//cert_${idx}.cert > server${idx}.log 2>&1 & " &
		((idx++))
	fi
	((count++))
done

while [ $count -gt 0 ]; do
wait $pids
count=`expr $count - 1`
done

done

echo "================ start done ======"
