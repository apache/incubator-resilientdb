export server=//benchmark/protocols/hs2:kv_server_performance
export TEMPLATE_PATH=$PWD/config/hs2.config

./performance/run_performance.sh $*

echo -e "${RED}PLEASE DO NOT FORGET TO STOP ALL AWS INSTANCES AFTER RUNNING ALL EXPERIMENTS USING THE FOLLOWING COMMAND:"
echo -e "${RED}../../stop_us_east_1_instances.sh${RESET}"