value_size=5400
req_num=1000
thd_num=1

bin=$1
config=$2
pri_key=$3
cert=$4
client_ip=$5

${bin} ${config} ${pri_key} ${cert} ${value_size} ${req_num} ${thd_num} ${client_ip}

