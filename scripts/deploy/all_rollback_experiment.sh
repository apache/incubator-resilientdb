#!/bin/bash

protocols=("HS-1" "HS-1-SLOT")
delays=(10 100)
num_slows=(0 1 4 7 10)
./start_us_east_1_instances.sh 31
# sleep 30

rm -rf ./plot_data/rollback_throughput
rm -rf ./plot_data/rollback_latency
mkdir -p ./plot_data/rollback_throughput
mkdir -p ./plot_data/rollback_latency

for delay in "${delays[@]}"; do
    for num_slow in "${num_slows[@]}"; do
        for protocol in "${protocols[@]}"; do
            if [[ "$protocol" != "HS-1-SLOT" && "$delay" != "100" ]]; then
                continue
            fi
            ./rollback_experiment.sh ${protocol} ${num_slow} ${delay}
            tail -n 2 results.log | head -n 1 > ./plot_data/rollback_throughput/${protocol}_${num_slow}_${delay}.data    
            tail -n 1 results.log > ./plot_data/rollback_latency/${protocol}_${num_slow}_${delay}.data    
        done
    done
done


./stop_us_east_1_instances.sh

rm -rf ./latex_plot_data/rollback_throughput.data
rm -rf ./latex_plot_data/rollback_latency.data
echo "n HS HS-2 HS-1 HS-1-SLOT10 HS-1-SLOT100" > ./latex_plot_data/rollback_throughput.data
echo "n HS HS-2 HS-1 HS-1-SLOT10 HS-1-SLOT100" > ./latex_plot_data/rollback_latency.data

for num_slow in "${num_slows[@]}"; do
    tput_str="${num_slow} -1000 -1000 "
    lat_str="${num_slow} -1 -1 "
    for protocol in "${protocols[@]}"; do
        for delay in "${delays[@]}"; do        
            if [[ "$protocol" != "HS-1-SLOT" && "$delay" != "100" ]]; then
                continue
            fi
            tput_str+=$(head -n 1 ./plot_data/rollback_throughput/${protocol}_${num_slow}_${delay}.data | awk '{print $1}')" "
            lat_str+=$(head -n 1 ./plot_data/rollback_latency/${protocol}_${num_slow}_${delay}.data | awk '{print $1}')" "
        done
    done
    echo "$tput_str" >> ./latex_plot_data/rollback_throughput.data
    echo "$lat_str" >> ./latex_plot_data/rollback_latency.data
done

./multiply_by_1000.sh ./latex_plot_data/rollback_latency.data