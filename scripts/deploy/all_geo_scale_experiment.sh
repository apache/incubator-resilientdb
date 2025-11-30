#!/bin/bash

protocols=("HS" "HS-2" "HS-1" "HS-1-SLOT")
num_regions=(2 3 4 5)
workloads=("ycsb" "tpcc")
./start_us_east_1_instances.sh 16
./start_eu_west_2_instances.sh 16
./start_ap_east_1_instances.sh 10
./start_sa_east_1_instances.sh 8
./start_eu_central_2_instances.sh 6

sleep 30

for workload in "${workloads[@]}"; do
    for num_region in "${num_regions[@]}"; do
        for protocol in "${protocols[@]}"; do
            ./geo_scale_experiment.sh ${protocol} ${num_region} ${workload}
            tail -n 2 results.log | head -n 1 > ./plot_data/geo_scale_${workload}_throughput/${protocol}_${num_region}.data    
            tail -n 1 results.log > ./plot_data/geo_scale_${workload}_latency/${protocol}_${num_region}.data    
        done
    done
done

./stop_all_instances.sh

for workload in "${workloads[@]}"; do
    rm -rf ./latex_plot_data/geo_scale_${workload}_throughput.data
    rm -rf ./latex_plot_data/geo_scale_${workload}_latency.data
    echo "n HS HS-2 HS-1 HS-1-SLOT" > ./latex_plot_data/geo_scale_${workload}_throughput.data
    echo "n HS HS-2 HS-1 HS-1-SLOT" > ./latex_plot_data/geo_scale_${workload}_latency.data
    for num_region in "${num_regions[@]}"; do
        tput_str="${num_region} "
        lat_str="${num_region} "
        for protocol in "${protocols[@]}"; do
            tput_str+=$(head -n 1 ./plot_data/geo_scale_${workload}_throughput/${protocol}_${num_region}.data)" "
            lat_str+=$(head -n 1 ./plot_data/geo_scale_${workload}_latency/${protocol}_${num_region}.data)" "
        done
        echo "$tput_str" >> ./latex_plot_data/geo_scale_${workload}_throughput.data
        echo "$lat_str" >> ./latex_plot_data/geo_scale_${workload}_latency.data
    done
done

