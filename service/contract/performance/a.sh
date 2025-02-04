for((i=0;i<100;i++))
do
`bazel run erc_eo_2pl_trans_performance > a.log 2>&1`
ret=$?
if [ $ret -gt 0 ]
then
exit 
fi
echo "done:$i" $ret
done
