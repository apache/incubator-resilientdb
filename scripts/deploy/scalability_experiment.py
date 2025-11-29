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
    mean_network_delay = 0,
    timer_length = 10): 

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
        "mean_network_delay": str(mean_network_delay),
        "timer_length": str(timer_length),
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


def generate_performance_server_conf(k, input_file = "./config/us-east-1-machines", output_file = "./config/performance.conf"):
    """
    Reads the first k lines from us-east-1-machines and writes them into performance.conf

    :param input_file: Path to the source file
    :param output_file: Path to the destination file
    :param k: Number of lines to copy
    """
    with open(input_file, "r") as infile, open(output_file, "w") as outfile:
        outfile.write("iplist=(\n")
        for i, line in enumerate(infile):
            if i >= k + 1:  # One more line for one client
                break
            outfile.write(line)
        outfile.write(")\n\nclient_num=1\nkey="+ssh_key)



def main():
    parser = argparse.ArgumentParser(description="Read protocol and replica number")
    parser.add_argument("protocol", type=str, help="Protocol name (string)")
    parser.add_argument("replica_number", type=int, help="Number of replicas (integer)")

    args = parser.parse_args()

    protocol = args.protocol
    replica_number = args.replica_number

    command, config_path, max_process_txn = get_experiment_command_and_config_and_maxprocesstxn(protocol)
    generate_config(config_path = config_path, max_process_txn = max_process_txn)

    command = command + " ./config/performance.conf"

    # print(f"command: {command}")

    generate_performance_server_conf(replica_number)

    print(f"{command}")


if __name__ == "__main__":
    main()
