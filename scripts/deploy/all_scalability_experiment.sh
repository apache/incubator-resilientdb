#!/bin/bash

protocols=("HS" "HS-2" "HS-1" "HS-1-SLOT")
replica_numbers=(4 16 32 64)

rm -rf ./plot_data/scalability_throughput
rm -rf ./plot_data/scalability_latency
mkdir -p ./plot_data/scalability_throughput
mkdir -p ./plot_data/scalability_latency
for replica_number in "${replica_numbers[@]}"; do
    ./start_us_east_1_instances.sh ${replica_number}

    sleep 30

    for protocol in "${protocols[@]}"; do
        ./scalability_experiment.sh ${protocol} ${replica_number}
        tail -n 2 results.log | head -n 1 > ./plot_data/scalability_throughput/${protocol}_${replica_number}.data    
        tail -n 1 results.log > ./plot_data/scalability_latency/${protocol}_${replica_number}.data                 
    done

done

./stop_us_east_1_instances.sh

rm -rf ./latex_plot_data/scalability_throughput.data
rm -rf ./latex_plot_data/scalability_latency.data

echo "n HS HS-2 HS-1 HS-1-SLOT" > ./latex_plot_data/scalability_throughput.data
echo "n HS HS-2 HS-1 HS-1-SLOT" > ./latex_plot_data/scalability_latency.data

# Read all the results
for replica_number in "${replica_numbers[@]}"; do
    tput_str="${replica_number} "
    lat_str="${replica_number} "
    
    for protocol in "${protocols[@]}"; do
        tput_str+=$(head -n 1 ./plot_data/scalability_throughput/${protocol}_${replica_number}.data | awk '{print $1}')" "
        lat_str+=$(head -n 1 ./plot_data/scalability_latency/${protocol}_${replica_number}.data | awk '{print $1}')" "
    done
    
    echo "$tput_str" >> ./latex_plot_data/scalability_throughput.data
    echo "$lat_str" >> ./latex_plot_data/scalability_latency.data
done

./multiply_by_1000.sh ./latex_plot_data/scalibility_latency.data