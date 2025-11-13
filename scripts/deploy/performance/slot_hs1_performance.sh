export server=//benchmark/protocols/slot_hs1:kv_server_performance
export TEMPLATE_PATH=$PWD/config/slot_hs1.config
RED="\033[31m"
RESET="\033[0m"


./performance/run_performance.sh $*

echo -e "${RED}PLEASE DO NOT FORGET TO STOP ALL AWS INSTANCES AFTER RUNNING ALL EXPERIMENTS USING THE FOLLOWING COMMAND:"
echo -e "${RED}./stop_us_east_1_instances.sh${RESET}"

echo -e "${RED}If YOU ARE RUNNING THE GEO-SCALE EXPERIMENT OR THE GEOGRAPHICAL DEPLOYMENT EXPERIMENT, PLEASE USE THE FOLLOWING COMMAND INSTEAD:${RESET}"
echo -e "${RED}./stop_all_instances.sh${RESET}"