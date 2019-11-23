#!/bin/bash
# RES_FILE --> Name of the result file.
#
USERNAME=expo
HOSTS="$1"
NODE_CNT="$2"
RES_FILE="$3"
count=0
for HOSTNAME in ${HOSTS}; do
	i=1
	if [ $count -ge $NODE_CNT ]; then
		i=0
	    SCRIPT="./runcl -nid${count} > ${RES_FILE}${count}.out 2>&1"
	    echo "${HOSTNAME}: runcl ${count}"
	else
	    SCRIPT="./rundb -nid${count} > ${RES_FILE}${count}.out 2>&1"
	    echo "${HOSTNAME}: rundb ${count}"
	fi
	# if [ "$i" -eq 0 ];then
		ssh -n -o BatchMode=yes -o StrictHostKeyChecking=no -l ${USERNAME} ${HOSTNAME} "
		cd resilientdb
		${SCRIPT}" &
	# fi
	count=`expr $count + 1`
done

while [ $count -gt 0 ]; do
	wait $pids
	count=`expr $count - 1`
done
