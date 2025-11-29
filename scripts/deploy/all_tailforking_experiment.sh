#!/bin/bash

protocols=("HS" "HS-2" "HS-1" "HS-1-SLOT")
delays=(10 100)
num_slows=(0 1 4 7 10)
./start_us_east_1_instances.sh 31
sleep 30

rm -rf ./plot_data/tail_forking_throughput
rm -rf ./plot_data/tail_forking_latency
mkdir -p ./plot_data/tail_forking_throughput
mkdir -p ./plot_data/tail_forking_latency

for delay in "${delays[@]}"; do
    for num_slow in "${num_slows[@]}"; do
        for protocol in "${protocols[@]}"; do
            if [[ "$protocol" != "HS-1-SLOT" && "$delay" != "100" ]]; then
                continue
            fi
            ./tail_forking_experiment.sh ${protocol} ${num_slow} ${delay}
            tail -n 2 results.log | head -n 1 > ./plot_data/tail_forking_throughput/${protocol}_${num_slow}_${delay}.data    
            tail -n 1 results.log > ./plot_data/tail_forking_latency/${protocol}_${num_slow}_${delay}.data    
        done
    done
done

# ./stop_us_east_1_instances.sh

rm -rf ./latex_plot_data/tail_forking_throughput.data
rm -rf ./latex_plot_data/tail_forking_latency.data
echo "n HS HS-2 HS-1 HS-1-SLOT10 HS-1-SLOT100" > ./latex_plot_data/tail_forking_throughput.data
echo "n HS HS-2 HS-1 HS-1-SLOT10 HS-1-SLOT100" > ./latex_plot_data/tail_forking_latency.data

for num_slow in "${num_slows[@]}"; do
    tput_str="${num_slow} "
    lat_str="${num_slow} "
    for protocol in "${protocols[@]}"; do
        for delay in "${delays[@]}"; do   
            if [[ "$protocol" != "HS-1-SLOT" && "$delay" != "100" ]]; then
                continue
            fi
            tput_str+=$(head -n 1 ./plot_data/tail_forking_throughput/${protocol}_${num_slow}_${delay}.data | awk '{print $1}')" "
            lat_str+=$(head -n 1 ./plot_data/tail_forking_latency/${protocol}_${num_slow}_${delay}.data | awk '{print $1}')" "
        done
    done
    echo "$tput_str" >> ./latex_plot_data/tail_forking_throughput.data
    echo "$lat_str" >> ./latex_plot_data/tail_forking_latency.data
done

./multiply_by_1000.sh ./latex_plot_data/tail_forking_latency.data