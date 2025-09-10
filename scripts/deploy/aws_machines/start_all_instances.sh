#!/bin/bash

./start_eu_central_2_instances.sh 6
./start_sa_east_1_instances.sh 8
./start_eu_west_2_instances.sh 10
./start_ap_east_1_instances.sh 16
./start_us_east_1_instances.sh 64

echo "GREAT! ALL INSTANCES IN ALL 5 REGIONS ARE STARTED NOW. THANK YOU FOR YOUR PATIENCE."