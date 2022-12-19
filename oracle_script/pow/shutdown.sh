#input="/home/ubuntu/nexres/oracle_script/pow/rep_120/svr_list.txt"
input=$1

count=0

set -x
HOSTS=`cat ${input}`
i=0
for HOSTNAME in ${HOSTS}; do
	if [ $i -eq 1 ]; then
		ssh -i /home/ubuntu/nexres/oracle_script/ssh-2022-03-24.key -n -o BatchMode=yes -o StrictHostKeyChecking=no ubuntu@${HOSTNAME} " shutdown; " &
		i=0
	else
		i=1
	fi
	count=`expr $count + 1`
done

while [ $count -gt 0 ]; do
	wait $pids
	count=`expr $count - 1`
done
