
iplist=`cat iplist.txt`
key=~/.ssh/junchao.pem

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

count=1
idx=1
for ip in ${iplist[@]};
do
 scp -i ${key} /home/ubuntu/nexres/bazel-bin/application/poe_mac/kv_server_performance ubuntu@${ip}:/home/ubuntu &
scp -i ${key} /home/ubuntu/nexres/oracle_script/poe_mac/rep_16/server.config ubuntu@${ip}:/home/ubuntu &
scp -i ${key} /home/ubuntu/nexres/oracle_script/poe_mac/rep_16/cert/node_${idx}.key.pri ubuntu@${ip}:/home/ubuntu/pbft_cert/ &
scp -i ${key} /home/ubuntu/nexres/oracle_script/poe_mac/rep_16/cert/cert_${idx}.cert ubuntu@${ip}:/home/ubuntu/pbft_cert/ &
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

idx=1
count=1
for ip in ${iplist[@]};
do
	ssh -i ${key} -n -o BatchMode=yes -o StrictHostKeyChecking=no ubuntu@${ip} " nohup /home/ubuntu/kv_server_performance /home/ubuntu/server.config pbft_cert//node_${idx}.key.pri pbft_cert//cert_${idx}.cert > server${idx}.log 2>&1 & " &
	((count++))
	((idx++))
done

while [ $count -gt 0 ]; do
wait $pids
count=`expr $count - 1`
done

echo "================ start done ======"
