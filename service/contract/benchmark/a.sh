for((i=0;i<1000;i++))
do
#bazel run :bam_trans_performance > a.log 2>&1
bazel run :bam_smallbank_performance > a.log 2>&1
ret=$?
echo $i,$ret
if [ $ret != 0 ]; then
exit 0
fi
done
