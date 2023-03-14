#!/bin/bash
USERNAME=ubuntu
HOSTS="$1"
SCRIPT="$2"
count=0
for HOSTNAME in ${HOSTS}; do
	ssh -n -o BatchMode=yes -o StrictHostKeyChecking=no -l ${USERNAME} ${HOSTNAME} "${SCRIPT}" &
	count=`expr $count + 1`
done

while [ $count -gt 0 ]; do
	wait $pids
	count=`expr $count - 1`
done
