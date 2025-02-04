bs_list=(500)
threads=(16)
thetas=$(seq 0 0.1 1)
#thetas=$(seq 0 0.2 4)
#thetas=$(seq 0.8 0.2 0.8)

for bs in ${bs_list[@]}
do

for th in ${threads[@]}
do

for t in ${thetas[@]}
do
#bazel run :occ_ycsb_main -- -threads $th -P /home/ubuntu/nexres/service/contract/benchmark/ycsb/workloads/workloada.spec -worker 16 -db occ -theta $t -batchsize $bs
bazel run :bam_ycsb_main -- -threads $th -P /home/ubuntu/nexres/service/contract/benchmark/ycsb/workloads/workloada.spec -worker 16 -db bam -theta $t -batchsize $bs
#bazel run :bam_ycsb_main -- -threads $th -P /home/ubuntu/nexres/service/contract/benchmark/ycsb/workloads/workloada.spec -worker 16 -db xp -theta $t -batchsize $bs
#bazel run :bam_ycsb_main -- -threads $th -P /home/ubuntu/nexres/service/contract/benchmark/ycsb/workloads/workloada.spec -worker 16 -db 2pl -theta $t -batchsize $bs

done

done

done
