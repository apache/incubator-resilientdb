#!/bin/bash

protocols=("HS" "HS-2" "HS-1" "HS-1-SLOT")
num_slows=(0 1 4 7 10)
delays=(10 100)
./start_us_east_1_instances.sh 31
sleep 30

for delay in "${delays[@]}"; do
    rm -rf ./plot_data/leader_slowness_${delay}_throughput
    rm -rf ./plot_data/leader_slowness_${delay}_latency
    mkdir -p ./plot_data/leader_slowness_${delay}_throughput
    mkdir -p ./plot_data/leader_slowness_${delay}_latency
    for num_slow in "${num_slows[@]}"; do
        for protocol in "${protocols[@]}"; do
            ./leader_slowness_experiment.sh ${protocol} ${num_slow} ${delay}
            tail -n 2 results.log | head -n 1 > ./plot_data/leader_slowness_${delay}_throughput/${protocol}_${num_slow}.data    
            tail -n 1 results.log > ./plot_data/leader_slowness_${delay}_latency/${protocol}_${num_slow}.data    
        done
    done
done


./stop_us_east_1_instances.sh


for delay in "${delays[@]}"; do
    rm -rf ./latex_plot_data/leader_slowness_${delay}_throughput.data
    rm -rf ./latex_plot_data/leader_slowness_${delay}_latency.data
    echo "n HS HS-2 HS-1 HS-1-SLOT" > ./latex_plot_data/leader_slowness_${delay}_throughput.data
    echo "n HS HS-2 HS-1 HS-1-SLOT" > ./latex_plot_data/leader_slowness_${delay}_latency.data
    for num_slow in "${num_slows[@]}"; do
        tput_str="${num_slow} "
        lat_str="${num_slow} "
        for protocol in "${protocols[@]}"; do
            tput_str+=$(head -n 1 ./plot_data/leader_slowness_${delay}_throughput/${protocol}_${num_slow}.data | awk '{print $1}')" "
            lat_str+=$(head -n 1 ./plot_data/leader_slowness_${delay}_latency/${protocol}_${num_slow}.data | awk '{print $1}')" "
        done
        echo "$tput_str" >> ./latex_plot_data/leader_slowness_${delay}_throughput.data
        echo "$lat_str" >> ./latex_plot_data/leader_slowness_${delay}_latency.data
    done
    ./multiply_by_1000.sh ./latex_plot_data/leader_slowness_${delay}_latency.data
done

