#!/bin/bash

./stop_us_east_1_instances.sh
./stop_ap_east_1_instances.sh
./stop_eu_central_2_instances.sh
./stop_sa_east_1_instances.sh
./stop_eu_west_2_instances.sh

echo "GREAT! ALL INSTANCES IN ALL 5 REGIONS ARE STOPPED NOW. THANK YOU FOR YOUR PATIENCE."