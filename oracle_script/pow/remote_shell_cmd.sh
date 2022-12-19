cmds=$1

count=0

echo $#
for cmd in "$*"; do
	echo "cmd"
	ssh -i /home/ubuntu/nexres/oracle_script/ssh-2022-03-24.key -n -o BatchMode=yes -o StrictHostKeyChecking=no ubuntu@${HOSTNAME} "$cmd" &
	count=`expr $count + 1`
done

while [ $count -gt 0 ]; do
	wait $pids
	count=`expr $count - 1`
done
