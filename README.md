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

For the convenience of reviewers, we have already created the instances required for the experiments and included their private IP addresses and instance IDs in the provided scripts for starting instances, stopping instances, and generating experiment configuration files.  

> **Note:** The paper mentions using *c3.4xlarge* instances. However, since these are an older generation and no longer widely available, we use *c4.4xlarge* instances instead.

# Build and Deploy ResilientDB

For the convenience of ARI reviewers, we provide a pre-configured AWS host machine with the following public IP address (**To save fundings, we have turned this instance off, please contact us (dakang@ucdavis.edu) to turn it on.**):

```bash
18.215.156.88
```
​   
You can access this host using the attached private key `hs1-ari.pem` (available in **Code & Scripts / Data** of the submission). To log in, first set the correct permissions on the key, then establish the SSH connection:

```bash
chmod 400 ./your_path_to/hs1-ari.pem
ssh -i ./your_path_to/hs1-ari.pem ubuntu@18.215.156.88
```

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

# Plotting Performance Results

We provide a separate LaTeX project, [`HotStuff-1-Plots`](https://github.com/DakaiKang/HotStuff-1-Plots), which allows reviewers to easily plot the performance results.  

To use it:  
1. Download the repository.  
2. Follow the instructions in the following parts of this README file.  
3. Compile the Latex project.
4. The figures with performance results will be available in the generated pdf.

This project is designed to reproduce the plots from our experiments with minimal effort.

# Scalability Experiments

## Throughput and Latency (Figure 7a, 7b)

In this experiment, we evaluate the performance of HotStuff-1 and baseline protocols under varying numbers of replicas. 

#### Step 1: Start AWS Instances
For this experiment, we use machines located in the **us-east-1** (North Virginia) region.  
To launch instances, use the provided script:

```bash
./start_us_east_1_instances.sh 4
```

The above command starts 4 AWS instances.
To start more replicas (e.g., 16), run:

```bash
./start_us_east_1_instances.sh 16
```

#### Step 2: Run the Scalability Experiment

Use the following script, replacing `protocol_name` and `replica_number` with your desired values:

```
./scalability_experiment.sh protocol_name replica_number
```

`protocol_name`: one of `[HS-1, HS-2, HS, HS-1-SLOT]`

`replica_number`: one of `[4, 16, 32, 64]`


#### Step 3: Collect Throughput and Latency Data

At the end of running the script above, the average **throughput** and **latency** of the run will be displayed.  
Record these values by adding them to `scripts/deploy/results_data/figure1_ab_scalability.py` under the corresponding `protocol_name` and `replica_number`.  

After collecting data for all parameter pairs, generate a table of data by running:

```bash
python3 ./results_data/figure1_ab_scalability.py
```

Next, copy the output into `data_scalability.tex` in the `HotStuff-1-Plots` project and recompile the LaTeX project.
The updated plots will then appear in `Figure 1 (a, b)`.

Stop all of the running instances aftre finishing this experiment.

#### Step 4: Stop the instances

Please stop the instances after running this set of experiments.

```bash
./stop_us_east_1_instances.sh
```

## Batching (Figure 7c, 7d)

In this experiment, we evaluate the performance of HotStuff-1 and baseline protocols under varying batchsize. 

#### Step 1: Start AWS Instances
For this experiment, we use 32 machines located in the **us-east-1** (North Virginia) region.  
To launch instances, use the provided script:

```bash
./start_us_east_1_instances.sh 32
```

#### Step 2: Run the Batching Experiment

Use the following script, replacing `protocol_name` and `replica_number` with your desired values:

```
./batching_experiment.sh protocol_name batch_size
```

`protocol_name`: one of `[HS-1, HS-2, HS, HS-1-SLOT]`

`batch_size`: one of `[100, 500, 1000, 5000, 10000]`


#### Step 3: Collect Throughput and Latency Data

At the end of running the script above, the average **throughput** and **latency** of the run will be displayed.  
Record these values by adding them to `scripts/deploy/results_data/figure1_cd_batching.py` under the corresponding `protocol_name` and `batch_size`.  

After collecting data for all parameter pairs, generate a table of data by running:

```bash
python3 ./results_data/figure1_cd_batching.py
```

Next, copy the output into `data_batching.tex` in the `HotStuff-1-Plots` project and recompile the LaTeX project.
The updated plots will then appear in `Figure 1 (c, d)`.

#### Step 4: Stop the instances

Please stop the instances after running this set of experiments.

```bash
./stop_us_east_1_instances.sh
```

## Geo-Scale + YCSB Benchmark (Figrue 7e, 7f)

In this experiment, we evaluate the performance of HotStuff-1 and baseline protocols under **geo-distributed settings**.  

We use **32 AWS machines**, evenly distributed across multiple regions.  
The table below shows the number of replicas assigned to each region for different region numbers.

| Number of Regions | West Virginia (us-east-1) | Hong Kong (ap-east-1) | London (eu-west-2) | San Paulo (sa-east-1) | Zurich (eu-central-2) |
| ----------------- | ------------------------- | --------------------- | ------------------ | --------------------- | --------------------- |
| 2                 | 16                        | 16                    |                    |                       |                       |
| 3                 | 11                        | 11                    | 10                 |                       |                       |
| 4                 | 8                         | 8                     | 8                  | 8                     |                       |
| 5                 | 7                         | 7                     | 6                  | 6                     | 6                     |


To enable communication between replicas across geo-distributed regions, we configure [VPC peering](https://docs.aws.amazon.com/vpc/latest/userguide/extend-intro.html).  
For the convenience of reviewers, this setup has already been completed in the AWS account provided for the ARI.  

#### Step 1: Start AWS Instances

To run geo-scale experiments, machines must be launched in multiple regions.  
For example, when running an experiment with **2 regions**, start **16 replicas** in `us-east-1` and **16 replicas** in `ap-east-1`.

    ./start_us_east_1_instances.sh 16
    ./start_ap_east_1_instances.sh 16

#### Step 2: Run the Geo-Scale Experiment

Use the following script, replacing `protocol_name` and `num_region` with your desired values:

```
./geo_scale_experiment.sh protocol_name num_region ycsb
```

`protocol_name`: one of `[HS-1, HS-2, HS, HS-1-SLOT]`

`num_region`: one of `[2, 3, 4, 5]`

#### Step 3: Collect Throughput and Latency Data

At the end of running the script above, the average **throughput** and **latency** of the run will be displayed.  
Record these values by adding them to `scripts/deploy/results_data/figure1_ef_geoscale_ycsb.py` under the corresponding `protocol_name` and `num_region`.  

After collecting data for all parameter pairs, generate a table of data by running:

```bash
python3 ./results_data/figure1_ef_geoscale_ycsb.py
```

Next, copy the output into `data_geo_scale_ycsb.tex` in the `HotStuff-1-Plots` project and recompile the LaTeX project.
The updated plots will then appear in `Figure 1 (e, f)`.

#### Step 4: Stop the instances

Please stop allthe instances after running this set of experiments.

```bash
./stop_all_instances.sh
```


## Geo-Scale + TPC-C Benchmark (Figrue 7g, 7h)

In this experiment, we evaluate the performance of HotStuff-1 and baseline protocols under **geo-distributed settings**, with **TPC-C** benchmark.  

We use **32 AWS machines**, evenly distributed across multiple regions.  

#### Step 0: Deploy TPC-C Database File and Prepare the Environment

Ensure that every replica has a copy of the **TPC-C database file** and has the necessary dependencies to execute the TPC-C transactions. 

```bash
# These steps can be time-consuming, but it only needs to be done once. For the reviewers’ convenience, this step has already been completed.
./start_all_instances.sh
./script/copy_tpcc_db.file.sh
./stop_all_instances.sh
```

#### Step 1: Start AWS Instances

The same as the Geo-Scale + YCSB experiments.

#### Step 2: Run the Geo-Scale Experiment

Use the following script, replacing `protocol_name` and `num_region` with your desired values:

```
./geo_scale_experiment.sh protocol_name num_region tpcc
```

`protocol_name`: one of `[HS-1, HS-2, HS, HS-1-SLOT]`

`num_region`: one of `[2, 3, 4, 5]`

#### Step 3: Collect Throughput and Latency Data

At the end of running the script above, the average **throughput** and **latency** of the run will be displayed.  
Record these values by adding them to `scripts/deploy/results_data/figure1_gh_geoscale_tpcc.py` under the corresponding `protocol_name` and `num_region`.  

After collecting data for all parameter pairs, generate a table of data by running:

```bash
python3 ./results_data/figure1_gh_geoscale_tpcc.py
```

Next, copy the output into `data_geo_scale_tpcc.tex` in the `HotStuff-1-Plots` project and recompile the LaTeX project.
The updated plots will then appear in `Figure 1 (g, h)`.

#### Step 4: Stop the instances

Please stop allthe instances after running this set of experiments.

```bash
./stop_all_instances.sh
```

## Network Delay Experiment (Figure 8 (a-d, f-i))

In this experiment, we evaluate the performance of HotStuff-1 and baseline protocols with injected message delay.

#### Step 1: Start AWS Instances
For this experiment, we use 31 machines located in the **us-east-1** (North Virginia) region.  
To launch instances, use the provided script:

```bash
./start_us_east_1_instances.sh 31
```

#### Step 2: Run the Network Delay Experiment

Use the following script, replacing `protocol_name`, `num_impacted_replica`, and `network_delay` with your desired values:

```
./network_delay_experiment.sh protocol_name num_impacted_replica network_delay
```

`protocol_name`: one of `[HS-1, HS-2, HS, HS-1-SLOT]`

`num_impacted_replica`: one of `[0, 10, 11, 20, 21, 31]`

`network_delay`: one of `[1, 5, 50, 500]`


#### Step 3: Collect Throughput and Latency Data (Taking 1-ms delay as example)

At the end of running the script above, the average **throughput** and **latency** of the run will be displayed.  
Record these values by adding them to `scripts/deploy/results_data/figure2_af_networkdelay_1ms.py` under the corresponding `protocol_name` and `num_impacted_replica`.  

After collecting data for all parameter pairs, generate a table of data by running:

```bash
python3 ./results_data/figure2_af_networkdelay_1ms.py
```

Next, copy the output into `data_network_delay_1ms.tex` in the `HotStuff-1-Plots` project and recompile the LaTeX project.
The updated plots will then appear in `Figure 2 (a, f)`. 
> **Note:** In the published version of the paper, latency was incorrectly labeled as `ms` rather than `s`.

#### Step 4: Stop the instances

Please stop the instances after running this set of experiments.

```bash
./stop_us_east_1_instances.sh
```

## Geographical Deployment (Figure 8e, Figure 8j)

In this experiment, we evaluate the performance of HotStuff-1 and baseline protocols when the replicas are located in two geographical regions.

#### Step 1: Start AWS Instances
For this experiment, we use 31 machines located in the **us-east-1** (North Virginia) region and  **us-west-2** (London) region.  
To launch instances, use the provided script:

```bash
./start_us_east_1_instances.sh 31
./start_eu_west_2_instances.sh 31
```

#### Step 2: Run the Geographical Deployment Experiment

Use the following script, replacing `protocol_name` and `num_london_replicas` with your desired values:

```
./geographical_deployment_experiment.sh protocol_name num_london_replicas
```

`protocol_name`: one of `[HS-1, HS-2, HS, HS-1-SLOT]`

`num_london_replicas`: one of `[0, 10, 11, 20, 21, 31]`

#### Step 3: Collect Throughput and Latency Data 

At the end of running the script above, the average **throughput** and **latency** of the run will be displayed.  
Record these values by adding them to `scripts/deploy/results_data/figure2_ej_geographical.py` under the corresponding `protocol_name` and `num_london_replicas`.  

After collecting data for all parameter pairs, generate a table of data by running:

```bash
python3 results_data/figure2_ej_geographical.py
```

Next, copy the output into `data_geographical_deployment.tex` in the `HotStuff-1-Plots` project and recompile the LaTeX project.
The updated plots will then appear in `Figure 2 (e, j)`.

#### Step 4: Stop the instances

Please stop the instances after running this set of experiments.

```bash
./stop_us_east_1_instances.sh
./stop_eu_west_2_instances.sh
```

## Leader Slowness Experiment (Figure 9 a-d)

In this experiment, we evaluate the performance of HotStuff-1 and baseline protocols with leader slowness.

#### Step 1: Start AWS Instances
For this experiment, we use 31 machines located in the **us-east-1** (North Virginia) region.  
To launch instances, use the provided script:

```bash
./start_us_east_1_instances.sh 31
```

#### Step 2: Run the Leader Slowness Experiment

Use the following script, replacing `protocol_name`, `num_slow_leaders`, and `timer_length` with your desired values:

```
./leader_slowness_experiment.sh protocol_name num_slow_leaders timer_length
```

`protocol_name`: one of `[HS-1, HS-2, HS, HS-1-SLOT]`

`num_slow_leaders`: one of `[0, 1, 4, 7, 10]`

`timer_length`: one of `[10, 100]`


#### Step 3: Collect Throughput and Latency Data (Taking 10-ms timer length as example)

At the end of running the script above, the average **throughput** and **latency** of the run will be displayed.  
Record these values by adding them to `scripts/deploy/results_data/figure3_ab_leader_slowness_10ms.py` under the corresponding `protocol_name` and `num_slow_leaders`.  

After collecting data for all parameter pairs, generate a table of data by running:

```bash
python3 ./results_data/figure3_ab_leader_slowness_10ms.py
```

Next, copy the output into `data_leader_slowness_10ms.tex` in the `HotStuff-1-Plots` project and recompile the LaTeX project.
The updated plots will then appear in `Figure 3 (a, b)`.

#### Step 4: Stop the instances

Please stop the instances after running this set of experiments.

```bash
./stop_us_east_1_instances.sh
```

## Tail-Forking Experiment (Figure 9e, Figure 9f)

In this experiment, we evaluate the performance of HotStuff-1 and baseline protocols under tail-forking attacks.

#### Step 1: Start AWS Instances
For this experiment, we use 31 machines located in the **us-east-1** (North Virginia) region.  
To launch instances, use the provided script:

```bash
./start_us_east_1_instances.sh 31
```

#### Step 2: Run the Tail-Forking Experiment

Use the following script, replacing `protocol_name`, `num_faulty_leaders`, and `timer_length` with your desired values:

```
./tail_forking_experiment.sh protocol_name num_faulty_leaders timer_length
```

`protocol_name`: one of `[HS-1, HS-2, HS, HS-1-SLOT]`

`num_faulty_leaders`: one of `[0, 1, 4, 7, 10]`

`timer_length`: for HS-1-SLOT: one of `[10, 100]`; for others: `[100]`


#### Step 3: Collect Throughput and Latency Data

At the end of running the script above, the average **throughput** and **latency** of the run will be displayed.  
Record these values by adding them to `scripts/deploy/results_data/figure3_ef_tailforking.py` under the corresponding `protocol_name` and `num_faulty_leaders`.

After collecting data for all parameter pairs, generate a table of data by running:

```bash
python3 ./results_data/figure3_ef_tailforking.py
```

Next, copy the output into `data_tailforking.tex` in the `HotStuff-1-Plots` project and recompile the LaTeX project.
The updated plots will then appear in `Figure 3 (e, f)`.

#### Step 4: Stop the instances

Please stop the instances after running this set of experiments.

```bash
./stop_us_east_1_instances.sh
```

## Rollback Experiment (Figure 9g, Figure 9h)

In this experiment, we evaluate the performance of HotStuff-1 with rollbacks.

#### Step 1: Start AWS Instances
For this experiment, we use 31 machines located in the **us-east-1** (North Virginia) region.  
To launch instances, use the provided script:

```bash
./start_us_east_1_instances.sh 31
```

#### Step 2: Run the Rollback Experiment

Use the following script, replacing `protocol_name`, `num_rollback`, and `timer_length` with your desired values:

```
./rollback_experiment.sh protocol_name num_rollback timer_length
```

`protocol_name`: one of `[HS-1, HS-1-SLOT]`

`num_rollback`: one of `[0, 1, 4, 7, 10]`

`timer_length`: for HS-1-SLOT: one of `[10, 100]`; for HS-1: `[100]`


#### Step 3: Collect Throughput and Latency Data

At the end of running the script above, the average **throughput** and **latency** of the run will be displayed.  
Record these values by adding them to `scripts/deploy/results_data/figure3_gh_rollback.py` under the corresponding `protocol_name` and `num_faulty_leaders`.

After collecting data for all parameter pairs, generate a table of data by running:

```bash
python3 ./results_data/figure3_gh_rollback.py
```

Next, copy the output into `data_rollback.tex` in the `HotStuff-1-Plots` project and recompile the LaTeX project.
The updated plots will then appear in `Figure 3 (g, h)`.

#### Step 4: Stop the instances

Please stop the instances after running this set of experiments.

```bash
./stop_us_east_1_instances.sh
```
