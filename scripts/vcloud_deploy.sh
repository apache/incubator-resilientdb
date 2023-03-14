#!/bin/bash
# RES_FILE --> Name of the result file.
#
USERNAME=ubuntu
HOSTS="$1"
NODE_CNT="$2"
RES_FILE="$3"
count=0
for HOSTNAME in ${HOSTS}; do
	i=1
	# if [ $count -eq 0 ]; then
	# 	echo "skipping first node"
	if [ $count -ge $NODE_CNT ]; then
		i=0
	    SCRIPT="ulimit -n 4096;./runcl -nid${count} > ${RES_FILE}${count}.out 2>&1"
	    echo "${HOSTNAME}: runcl ${count}"
	else
	    SCRIPT="ulimit -n 4096;source /home/ubuntu/sgx/sgxsdk/environment && ./enclave > enclave_log & ./rundb -nid${count} > ${RES_FILE}${count}.out; pkill enclave 2>&1"
	    echo "${HOSTNAME}: rundb ${count}"
	fi
	# if [ "$i" -eq 0 ];then
		ssh -n -o BatchMode=yes -o StrictHostKeyChecking=no -l ${USERNAME} ${HOSTNAME} "
		cd resilientdb && bash -c '${SCRIPT}'" &
	# fi
	count=`expr $count + 1`
done

while [ $count -gt 0 ]; do
	wait $pids
	count=`expr $count - 1`
done
