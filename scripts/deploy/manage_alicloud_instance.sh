#!/bin/bash

# Sample: ./manage_alicloud_instance.sh new lan 22 write run shutdown
# Sample: ./manage_alicloud_instance.sh old lan 0 shutdown

# function: distribute the total number
distribute_evenly() {
    local total=$1
    local listsize=$2

    local base_value=$((total / listsize))
    local remainder=$((total % listsize))

    # 初始化数组并分配
    for ((i = 0; i < listsize; i++)); do
        if ((i < remainder)); then
            echo $((base_value + 1)) 
        else
            echo $base_value
        fi
    done
}

mode=$1
shift
network=$1
shift
amount=$1
shift
listsize=0
echo $mode, $network

region_list=('eu-central-1' 'us-west-1' 'ap-southeast-1' 'me-east-1')
imageid_list=('m-gw8atjzmmv35esf7t0ol' 'm-rj9hvjdzwmn5hszxi7rl' 'm-t4n9apvlakv21suofn9c' 'm-eb32jb9t0l0fabyiuswx')
zoneid_list=('eu-central-1a' 'us-west-1a' 'ap-southeast-1a' 'me-east-1a')
instance_type_list=('ecs.e-c1m4.large' 'ecs.e-c1m4.large' 'ecs.e-c1m4.large' 'ecs.g6.large')
security_groupid_list=('sg-gw882198cp93dc3he2pp' 'sg-rj91tgsgh70v6v1y0sqw' 'sg-t4ngrbld64k3fls433bj' 'sg-eb32jb9t0l0f9u76i0n0')
vpc_list=('vpc-gw87tw2ohh8amcdkvoeci' 'vpc-rj9jfm8myo5eug7gbbi31' 'vpc-t4nskgim87ibkhz04y4up' 'vpc-eb3nl8lrg1jh792ahzzny')
vswich_list=('vsw-gw818gr9injn3xq6wqvhv' 'vsw-rj9uoqiwyrorgxnrfr7tc' 'vsw-t4ny2ave4421x41cicpah' 'vsw-eb3ge3h2j2s0rsqwaigcy')
disk_cata_list=('cloud_auto' 'cloud_auto' 'cloud_auto' 'cloud_essd')

listsize=${#region_list[@]}

amount_list=($(distribute_evenly $amount $listsize))

if [[ $mode == "new" ]]; then
  if [[ $network == "lan" ]]; then
    region='cn-hongkong'
    imageid='m-j6c05t5ubgx5p2iokg53'
    zoneid='cn-hongkong-b'
    instance_type='ecs.g7t.large'
    security_groupid='sg-j6c983lbmrcoilavm6vf'
    vswichid='vsw-j6ctft5mb2zny49xzub9j'

    aliyun ecs RunInstances \
        --RegionId $region \
        --ImageId $imageid \
        --ZoneId $zoneid \
        --InstanceType $instance_type \
        --SecurityGroupId $security_groupid \
        --VSwitchId $vswichid \
        --InstanceName 'shaokang-fides-instance' \
        --InternetMaxBandwidthIn 100 \
        --InternetMaxBandwidthOut 100 \
        --HostName 'fides-instance' \
        --UniqueSuffix true \
        --InternetChargeType 'PayByTraffic' \
        --SystemDisk.Size 80 \
        --SystemDisk.Category cloud_auto \
        --Tag.1.Key name \
        --Tag.1.Value 'fides-instance' \
        --Amount $amount

    echo "sleep 60"
    sleep 60

  elif [[ $network == "wan" ]]; then


    # 遍历每个区域的配置并运行实例
    for ((i = 0; i < listsize; i++)); do
      region=${region_list[i]}
      imageid=${imageid_list[i]}
      zoneid=${zoneid_list[i]}
      instance_type=${instance_type_list[i]}
      security_groupid=${security_groupid_list[i]}
      vswichid=${vswich_list[i]}
      amount=${amount_list[i]}
      disk_cata=${disk_cata_list[i]}

      echo "$region, $imageid, $zoneid, $instance_type, $security_groupid, $vswichid, $amount"

      aliyun ecs RunInstances \
          --RegionId "$region" \
          --ImageId "$imageid" \
          --ZoneId "$zoneid" \
          --InstanceType "$instance_type" \
          --SecurityGroupId "$security_groupid" \
          --VSwitchId "$vswichid" \
          --InstanceName 'shaokang-fides-instance' \
          --InternetMaxBandwidthIn 100 \
          --InternetMaxBandwidthOut 100 \
          --HostName 'fides-instance' \
          --UniqueSuffix true \
          --InternetChargeType 'PayByTraffic' \
          --SystemDisk.Size 80 \
          --SystemDisk.Category "$disk_cata" \
          --Tag.1.Key name \
          --Tag.1.Value 'fides-instance' \
          --Amount $amount

      # wait for some time
      sleep 0.5
    done

    echo "sleep 60"
    sleep 60
  fi
fi


# Describe Instances
instance_ids=()
public_ips=()
private_ips=()

need_describe=false

for arg in "$@"; do
  if [[ "$arg" == "shutdown" || "$arg" == "write" ]]; then
    need_describe=true
    break
  fi
done

if [[ "$need_describe" == true ]]; then
  echo Need Describe.

  if [[ $network == "lan" ]]; then

    data=$(aliyun ecs DescribeInstances \
        --region cn-hongkong \
        --Tag.1.Key name \
        --Tag.1.Value fides-instance \
        --MaxResults 100 \
        --output cols=InstanceId,PublicIpAddress,VpcAttributes.PrivateIpAddress rows=Instances.Instance[])
        # --output cols=InstanceId,InstanceName,PublicIpAddress,VpcAttributes.PrivateIpAddress rows=Instances.Instance[])
    # echo $info

    # Sample return data
    # data="InstanceId | PublicIpAddress | VpcAttributes.PrivateIpAddress ---------- | --------------- | ------------------------------ i-j6c48395f9rus4psiw7y | map[IpAddress:[47.242.8.123]] | map[IpAddress:[172.18.218.174]] i-j6c48395f9rus4psiw7z | map[IpAddress:[47.242.220.33]] | map[IpAddress:[172.18.218.173]] i-j6c48395f9rus4psiw7x | map[IpAddress:[47.242.222.206]] | map[IpAddress:[172.18.218.175]]"

    # Replace 'i-' into '\n'
    formatted_data=$(echo "$data" | sed 's/i-/\ni-/g')

    # Iterate each row.
    while read -r line; do
      instance_id=$(echo "$line" | awk -F '|' '{print $1}' | xargs)
      instance_ids+=("$instance_id")
      public_ip=$(echo "$line" | awk -F '|' '{print $2}' | sed 's/.*IpAddress:\[\(.*\)\].*/\1/' | sed 's/]//g' | xargs)
      public_ips+=("$public_ip")
      private_ip=$(echo "$line" | awk -F '|' '{print $3}' | sed 's/.*IpAddress:\[\(.*\)\].*/\1/' | sed 's/]//g' | xargs)
      private_ips+=("$private_ip")
    #   echo "$instance_id, $public_ip, $private_ip"
    done <<< "$(echo "$formatted_data" | grep 'i-')"

    echo "Instance IDs: ${instance_ids[@]}"
    echo "Public IPs: ${public_ips[@]}"
    echo "Private IPs: ${private_ips[@]}"

  elif [[ $network == "wan" ]]; then
    amount_list_sd=()
    for region in "${region_list[@]}"; do
      data=$(aliyun ecs DescribeInstances \
          --region $region \
          --Tag.1.Key name \
          --Tag.1.Value fides-instance \
          --MaxResults 100 \
          --output cols=InstanceId,PublicIpAddress,VpcAttributes.PrivateIpAddress rows=Instances.Instance[])
      echo $data
      # Replace 'i-' into '\n'
      formatted_data=$(echo "$data" | sed 's/i-/\ni-/g')

      # Iterate each row.
      counter=-1
      while read -r line; do
        ((counter++))
        instance_id=$(echo "$line" | awk -F '|' '{print $1}' | xargs)
        instance_ids+=("$instance_id")
        public_ip=$(echo "$line" | awk -F '|' '{print $2}' | sed 's/.*IpAddress:\[\(.*\)\].*/\1/' | sed 's/]//g' | xargs)
        public_ips+=("$public_ip")
        private_ip=$(echo "$line" | awk -F '|' '{print $3}' | sed 's/.*IpAddress:\[\(.*\)\].*/\1/' | sed 's/]//g' | xargs)
        private_ips+=("$private_ip")
      done <<< "$(echo "$formatted_data" | grep 'i-')"
      ((counter++))
      amount_list_sd+=("$counter")
    done

    echo "amount_list_sd: ${amount_list_sd[@]}"
    echo "Instance IDs: ${instance_ids[@]}"
    echo "Public IPs: ${public_ips[@]}"
    echo "Private IPs: ${private_ips[@]}"
  fi

fi

if [[ $1 == "write" ]]; then
  shift
  # 定义文件名
  output_file="config/kv_performance_server.conf"
  client_num=$(( ${#instance_ids[@]} / 2 ))
  # 写入到文件中
  shuffled_ips=()
  if [[ $network == "wan" ]]; then
    shuffled_ips=($(shuf -e "${public_ips[@]}"))
  elif [[ $network == "lan" ]]; then
    shuffled_ips=($(shuf -e "${public_ips[@]}"))
  fi

  {
    echo "iplist=("
    for ip in "${shuffled_ips[@]}"; do
      echo "$ip"
    done
    echo ")"
    echo ""
    echo "client_num=$client_num"
  } > "$output_file"

  # 显示生成的文件内容
  cat "$output_file"
fi

if [[ $1 == "run" ]]; then
  shift
  if [[ $network == "lan" ]]; then
    bash ./start_evaluation.sh simulate
  elif [[ $network == "wan" ]]; then
    bash ./start_evaluation.sh simulate
  fi
fi


# amount_list=(1 9 7 7)
if [[ $1 == "shutdown" ]]; then
  # Form a delete command containing all the instance_ids.
  # Sample:
  # aliyun ecs DeleteInstances \
  #     --region cn-hongkong \
  #     --InstanceId.1 i-1 \
  #     --InstanceId.2 i-2 \
  #     --Force true
  
  if [[ $network == "lan" ]]; then
    counter=1
    delete_command="aliyun ecs DeleteInstances --region cn-hongkong"
    for instance_id in "${instance_ids[@]}"; do
        delete_command="$delete_command --InstanceId.$counter $instance_id"
        ((counter++))
    done
    delete_command="$delete_command --Force true"
    echo $delete_command
    $delete_command
  elif [[ $network == "wan" ]]; then
    echo shutdown in wan
    instance_index=0
    for ((i = 0; i < listsize; i++)); do
      if [[ ${amount_list_sd[$i]} -gt 0 ]]; then
        counter=1
        delete_command="aliyun ecs DeleteInstances --region ${region_list[$i]}"
        # for instance_id in "${instance_ids[@]}"; do

        for ((j = 0; j < ${amount_list_sd[$i]}; j++)); do
          delete_command="$delete_command --InstanceId.$counter ${instance_ids[$instance_index]}"
          ((counter++))
          ((instance_index++))
        done
        delete_command="$delete_command --Force true"
        echo $delete_command
        $delete_command
      fi
    done
  fi
fi