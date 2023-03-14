#!/bin/bash
# RES_FILE --> Name of the result file.
#
USERNAME=ubuntu
HOSTS="$1"
IPAddr="$2"


for HOSTNAME in ${HOSTS}; do
	ssh -n -o BatchMode=yes -o StrictHostKeyChecking=no -l ${USERNAME} ${HOSTNAME} "
	cd resilientdb
	(sh monitorResults.sh ${IPAddr}) > LogsOfMonitorScript.txt 2>&1" &
	
done
