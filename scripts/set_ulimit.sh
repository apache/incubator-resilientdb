#!/bin/bash
USERNAME=ubuntu
HOSTS="$1"

count=0
for HOSTNAME in ${HOSTS}; do
	ssh -n -o BatchMode=yes -o StrictHostKeyChecking=no -l ${USERNAME} ${HOSTNAME} "
  ulimit -c unlimited
  sudo service apport start
  rm /var/crash/*" & 
	count=`expr $count + 1`
done

