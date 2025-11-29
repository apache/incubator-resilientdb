#!/bin/bash

protocols=("HS" "HS-2" "HS-1" "HS-1-SLOT")
network_delays=(1 5 50 500)
num_impacted=(0 10 11 20 21 31)

rm -rf ./plot_data/network_delay_throughput
rm -rf ./plot_data/network_delay_latency
mkdir -p ./plot_data/network_delay_throughput
mkdir -p ./plot_data/network_delay_latency

./start_us_east_1_instances.sh 31
sleep 30

for network_delay in "${network_delays[@]}"; do
    for n in "${num_impacted[@]}"; do
        for protocol in "${protocols[@]}"; do
            ./network_delay_experiment.sh ${protocol} ${n} ${network_delay}
            tail -n 2 results.log | head -n 1 > ./plot_data/network_delay_throughput/${protocol}_${network_delay}_${n}.data    
            tail -n 1 results.log > ./plot_data/network_delay_latency/${protocol}_${network_delay}_${n}.data    
        done
                     
    done
done


./stop_us_east_1_instances.sh

# Read all the results
for network_delay in "${network_delays[@]}"; do
    rm -rf ./latex_plot_data/network_delay_${network_delay}_throughput.data
    rm -rf ./latex_plot_data/network_delay_${network_delay}_latency.data

    echo "n HS HS-2 HS-1 HS-1-SLOT" > ./latex_plot_data/network_delay_${network_delay}_throughput.data
    echo "n HS HS-2 HS-1 HS-1-SLOT" > ./latex_plot_data/network_delay_${network_delay}_latency.data
    for n in "${num_impacted[@]}"; do
        tput_str="${n} "
        lat_str="${n} "
        for protocol in "${protocols[@]}"; do
            tput_str+=$(head -n 1 ./plot_data/network_delay_throughput/${protocol}_${network_delay}_${n}.data | awk '{print $1}')" "
            lat_str+=$(head -n 1 ./plot_data/network_delay_latency/${protocol}_${network_delay}_${n}.data | awk '{print $1}')" "
        done
        echo "$tput_str" >> ./latex_plot_data/network_delay_${network_delay}_throughput.data
        echo "$lat_str" >> ./latex_plot_data/network_delay_${network_delay}_latency.data
    done
done