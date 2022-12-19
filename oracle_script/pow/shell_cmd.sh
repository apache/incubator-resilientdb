cmds=$1

count=0

echo $#
for cmd in "$@"; do
	echo $cmd | sh &
	count=`expr $count + 1`
done

while [ $count -gt 0 ]; do
	wait $pids
	count=`expr $count - 1`
done
