# Overview

This artifact contains our implementation of [HotStuff-1](https://dl.acm.org/doi/10.1145/3725308) along with several baseline protocols, all built on top of [Apache ResilientDB](https://resilientdb.incubator.apache.org/).  

It provides detailed instructions for reproducing the experimental results, including:

- Hardware configuration used in our evaluation  
- Steps to build and deploy ResilientDB  
- Scripts to run experiments and collect results  

# Hardware Configuration

We conduct our experiments on **AWS c4.4xlarge** instances with the following specifications:

- 16-core Intel Xeon E5-2666 v3 (Haswell) processor, up to 2.9 GHz  
- 30 GB memory  
- Ubuntu Server 22.04 LTS (HVM)  
- SSD storage (General Purpose)  

The AWS instances can be provisioned either:  

- Programmatically via the [AWS API](https://docs.aws.amazon.com/opsworks/latest/APIReference/API_CreateInstance.html) using [Access Keys](https://docs.aws.amazon.com/IAM/latest/UserGuide/security-creds.html), or  
- Manually through the AWS Management Console.  

> **Note:** The paper mentions using *c3.4xlarge* instances. However, since these are an older generation and no longer widely available, we use *c4.4xlarge* instances instead.



# Build and Deploy ResilientDB

### Set up the machines and scripts

Create instances on your cloud service provider (e.g., AWS).

Copy the **private IP addresses** of the instances into
 `scripts/deploy/config/{region_name}-machines` (for example, `us-east-1-machines`).

Copy the **instance IDs** into the scripts used to start and stop the instances, for example:
 `scripts/deploy/start_us_east_1_instances.sh`.

Replace all occurrences of `hs1-ari.pem` with the path to the SSH key associated with your instances.

After configuring your cloud credentials with permissions to start and stop instances, you can conveniently manage the instances using the provided scripts.



### Compile the code

Once logged in, clone the ResilientDB repository, switch to the hotstuff-1 branch, and install the dependencies.
(For reviewer convenience, these steps have already been executed on the host machine. You can directly enter the repository directory after logging in.)

```bash
git clone https://github.com/apache/incubator-resilientdb.git
cd incubator-resilientdb/
git checkout hotstuff-1
./INSTALL.sh
```

Next, enter the directory where the deployment scripts are located:

```
cd scripts/deploy/
```

By modifying the value at **line 28** of `scripts/deploy/performance/run_performance.sh`, you can adjust the duration of each experiment. In our experience, performance results stabilize after about **20 seconds**, but you may set a longer duration (e.g., **60 seconds** or longer) if desired.



### Running the Experiments

We provide scripts that make it easy to run experiments with different parameter configurations. Each script corresponds to a specific type of experiment:

- **Scalability Experiments:** `./all_scalability_experiment.sh`
- **Batching Experiments:** `./all_batching_experiment.sh`
- **Geo-Scale Experiments:** `./all_geo_scale_experiment.sh`
- **Network Delay Experiments:** `./all_network_delay_experiment.sh`
- **Geographical Deployment Experiments:** `./all_geographical_experiment.sh`
- **Leader Slowness Experiments:** `./all_leader_slowness_experiment.sh`
- **Tail-Forking Experiments:** `./all_tailforking_experiment.sh`
- **Rollback Experiments:** `./all_rollback_experiment.sh`

Each script automatically iterates over the relevant parameter combinations and runs the corresponding experiments.

All experiment results are collected in `scripts/deploy/plot_data`.

Experiment results of the same type are organized in `scripts/deploy/latex_plot_data` for easy plotting.



### Plotting Performance Results

We provide a separate LaTeX project, [`HotStuff-1-Plots`](https://github.com/DakaiKang/HotStuff-1-Plots), which allows reviewers to easily plot the performance results.  

To use it:  

1. Download the repository.  
2. Copy the results in `scripts/deploy/latex_plot_data` into the corresponding files.
3. Compile the Latex project.
4. The figures with performance results will be available in the generated pdf.

This project is designed to reproduce the plots from our experiments with minimal effort.
