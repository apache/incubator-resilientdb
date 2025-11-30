#!/bin/bash

protocols=("HS" "HS-2" "HS-1" "HS-1-SLOT")
batchings=(100 1000 2000 5000 10000)
./start_us_east_1_instances.sh 32
sleep 30

for batching in "${batchings[@]}"; do

    for protocol in "${protocols[@]}"; do
        ./batching_experiment.sh ${protocol} ${batching}
        tail -n 2 results.log | head -n 1 > ./plot_data/batching_throughput/${protocol}_${batching}.data    
        tail -n 1 results.log > ./plot_data/batching_latency/${protocol}_${batching}.data                 
    done

done

./stop_us_east_1_instances.sh

echo "n HS HS-2 HS-1 HS-1-SLOT" > ./latex_plot_data/batching_throughput.data
echo "n HS HS-2 HS-1 HS-1-SLOT" > ./latex_plot_data/batching_latency.data

# Read all the results
for batching in "${batchings[@]}"; do
    tput_str="${batching} "
    lat_str="${batching} "
    
    for protocol in "${protocols[@]}"; do
        tput_str+=$(head -n 1 ./plot_data/batching_throughput/${protocol}_${batching}.data | awk '{print $1}')" "
        lat_str+=$(head -n 1 ./plot_data/batching_latency/${protocol}_${batching}.data | awk '{print $1}')" "
    done
    
    echo "$tput_str" >> ./latex_plot_data/batching_throughput.data
    echo "$lat_str" >> ./latex_plot_data/batching_latency.data
done

./multiply_by_1000.sh ./latex_plot_data/batching_latency.data 