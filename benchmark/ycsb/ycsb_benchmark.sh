bs_list=(500)
threads=(16)
#thetas=$(seq 0 0.2 1)
thetas=$(seq 0 0.2 4)

for bs in ${bs_list[@]}
do

for th in ${threads[@]}
do

for t in ${thetas[@]}
do
bazel run :kv_ycsb_main -- -threads $th -P ~/resilientdb/benchmark/ycsb/workloads/workloada.spec -worker 16 -db occ -theta $t -batchsize $bs


done

done

done