import argparse
import subprocess
import os
import sys

ssh_key = "~/hs1-ari.pem"

def generate_config(
    config_path, 
    clientBatchNum = 100,
    enable_viewchange = False,
    recovery_enabled = False,
    max_client_complaint_num = 10,
    max_process_txn = 3,
    worker_num = 8,
    input_worker_num = 5,
    output_worker_num = 5,
    non_responsive_num = 0,
    fork_tail_num = 0,
    rollback_num = 0,
    tpcc_enabled = False,
    network_delay_num =  0,
    mean_network_delay = 0): 

    config_dict = {
        "clientBatchNum": str(clientBatchNum),
        "enable_viewchange": "true" if enable_viewchange else "false",
        "recovery_enabled": "true" if recovery_enabled else "false",
        "max_client_complaint_num": str(max_client_complaint_num),
        "max_process_txn": str(max_process_txn),
        "worker_num": str(worker_num),
        "input_worker_num": str(input_worker_num),
        "output_worker_num": str(output_worker_num),
        "non_responsive_num": str(non_responsive_num),
        "fork_tail_num": str(fork_tail_num),
        "rollback_num": str(rollback_num),
        "tpcc_enabled": "true" if tpcc_enabled else "false",
        "network_delay_num": str(network_delay_num),
        "mean_network_delay": str(mean_network_delay)
    }

    with open(config_path, "w") as f:
        f.write("{\n")
        items = list(config_dict.items())
        for i, (key, value) in enumerate(items):
            if i == len(items) - 1:  # last item
                f.write(f'  "{key}": {value}\n')
            else:
                f.write(f'  "{key}": {value},\n')
        f.write("}\n")



def get_experiment_command_and_config_and_maxprocesstxn(protocol):
    if protocol == "HS-1":
        return "./performance/hs1_performance.sh", "./config/hs1.config", 3
    elif protocol == "HS-2":
        return "./performance/hs2_performance.sh", "./config/hs2.config", 4
    elif protocol == "HS":
        return "./performance/hs_performance.sh", "./config/hs.config", 5
    elif protocol == "HS-1-SLOT":
        return "./performance/slot_hs1_performance.sh", "./config/slot_hs1.config", 3


def generate_performance_server_conf(region_number, output_file = "./config/performance.conf"):
    """
    Reads the first k lines from us-east-1-machines and writes them into performance.conf

    :param input_file: Path to the source file
    :param output_file: Path to the destination file
    :param k: Number of lines to copy
    """

    input_file_dict = dict()
    if region_number == 2:
        input_file_dict["./config/us-east-1-machines"] = 16
        input_file_dict["./config/ap-east-1-machines"] = 16
    
    elif region_number == 3:
        input_file_dict["./config/us-east-1-machines"] = 11
        input_file_dict["./config/ap-east-1-machines"] = 11
        input_file_dict["./config/eu-west-2-machines"] = 10

    elif region_number == 4:
        input_file_dict["./config/us-east-1-machines"] = 8
        input_file_dict["./config/ap-east-1-machines"] = 8
        input_file_dict["./config/eu-west-2-machines"] = 8
        input_file_dict["./config/sa-east-1-machines"] = 8
    
    elif region_number == 5:
        input_file_dict["./config/us-east-1-machines"] = 7
        input_file_dict["./config/ap-east-1-machines"] = 7
        input_file_dict["./config/eu-west-2-machines"] = 6
        input_file_dict["./config/sa-east-1-machines"] = 6
        input_file_dict["./config/eu-central-2-machines"] = 6


    with open(output_file, "w") as outfile:
        outfile.write("iplist=(\n")
        for filepath, k in input_file_dict.items():
            with open(filepath, "r") as infile: 
                for i, line in enumerate(infile):
                    if i >= k:  # One more line for one client
                        break
                    outfile.write(line)
        outfile.write("\n")
        with open("./config/us-east-1-machines", "r") as infile: 
                for i, line in enumerate(infile):
                    if i == 1 + input_file_dict["./config/us-east-1-machines"]:  # One more line for one client
                        outfile.write(line)
                        break
        outfile.write(")\n\nclient_num=1\nkey="+ssh_key)



def main():
    parser = argparse.ArgumentParser(description="Read protocol and replica number")
    parser.add_argument("protocol", type=str, help="Protocol name (string)")
    parser.add_argument("region_number", type=int, help="Number of regions (integer)")
    parser.add_argument("benchmark", type=str, help="Benchmark (string)")

    args = parser.parse_args()

    protocol = args.protocol
    region_number = args.region_number
    benchmark = args.benchmark

    if benchmark not in ["ycsb", "tpcc"]:
        print("benchmark should be ycsb or tpcc")
        sys.exit(1)

    tpcc_enabled = False if benchmark == "ycsb" else True

    command, config_path, max_process_txn = get_experiment_command_and_config_and_maxprocesstxn(protocol)
    generate_config(config_path = config_path, max_process_txn = max_process_txn, tpcc_enabled=tpcc_enabled)

    command = command + " ./config/performance.conf"

    generate_performance_server_conf(region_number)

    print(f"{command}")


if __name__ == "__main__":
    main()
