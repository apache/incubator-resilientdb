#!/bin/bash

protocols=("HS" "HS-2" "HS-1" "HS-1-SLOT")
num_londons=(0 10 11 20 21 31)
./start_us_east_1_instances.sh 31
./start_eu_west_2_instances.sh 31
sleep 30

rm -rf ./plot_data/geographical_throughput
rm -rf ./plot_data/geographical_latency
mkdir -p ./plot_data/geographical_throughput
mkdir -p ./plot_data/geographical_latency

for num_london in "${num_londons[@]}"; do
    for protocol in "${protocols[@]}"; do
        ./geographical_experiment.sh ${protocol} ${num_london}
        tail -n 2 results.log | head -n 1 > ./plot_data/geographical_throughput/${protocol}_${num_london}.data    
        tail -n 1 results.log > ./plot_data/geographical_latency/${protocol}_${num_london}.data                 
    done

done

./stop_all_instances.sh
rm -rf ./latex_plot_data/geographical_throughput.data
rm -rf ./latex_plot_data/geographical_latency.data

echo "n HS HS-2 HS-1 HS-1-SLOT" > ./latex_plot_data/geographical_throughput.data
echo "n HS HS-2 HS-1 HS-1-SLOT" > ./latex_plot_data/geographical_latency.data

# Read all the results
for num_london in "${num_londons[@]}"; do
    tput_str="${num_london} "
    lat_str="${num_london} "
    
    for protocol in "${protocols[@]}"; do
        tput_str+=$(head -n 1 ./plot_data/geographical_throughput/${protocol}_${num_london}.data)" "
        lat_str+=$(head -n 1 ./plot_data/geographical_latency/${protocol}_${num_london}.data)" "
    done
    
    echo "$tput_str" >> ./latex_plot_data/geographical_throughput.data
    echo "$lat_str" >> ./latex_plot_data/geographical_latency.data
done