# Usage

## Build and Deploy ResilientDB

This section describes how to quickly build ResilientDB and deploy 4 replicas along with 1 client proxy on your local machine.

The client proxy serves as the interface for all clients. It batches client requests and forwards these batches to the designated leader replica. The 4 replicas then participate in PBFT consensus to order and execute these batches. Once executed, the leader returns the responses to the proxy.

### Step 1: Install Dependencies

```bash
./INSTALL.sh
```

### Step 2: Run ResilientDB (Key-Value Service)

```bash
./service/tools/kv/server_tools/start_kv_service.sh
```

This script starts 4 replicas and 1 client. Each replica instantiates a key-value store instance.

### Step 3: Build Interactive Tools

```bash
bazel build service/tools/kv/api_tools/kv_service_tools
```

## Test Performance

To deploy ResilientDB across multiple machines, edit the file `scripts/deploy/config/kv_performance_server.conf` and add the **private IP addresses** of the target machines for replicas and the client proxy.

> ⚠️ Notes:
> 
> - If your machines do not require an SSH key for login, update the scripts under `scripts/deploy/script` accordingly.
> - The default user is assumed to be `ubuntu`, and the working directory `/home/ubuntu/`. Update the scripts if your environment differs.
> - If your binary is located in a different path, adjust the script paths as needed.

### Configuration

Before testing:

1. Populate `scripts/deploy/config/kv_performance_server.conf` with the private IPs.
2. Create a key file `config/key.conf` containing your SSH private key.
   - Refer to the example at `scripts/deploy/config/key_example.conf`.

### Navigate to the Deployment Directory

```bash
cd scripts/deploy
```

### Copy TPC-C Database Files to Replicas if Running with TPC-C Benchmark (Skip if Running With Default Key-Value Benchmark)

```bash
./script/copy_tpcc_db_file.sh
```

### Run Performance Benchmarks

- **HotStuff-1**:

```bash
./performance/hs1_performance.sh config/kv_performance_server.conf
```

- **HotStuff**:

```bash
./performance/hs_performance.sh config/kv_performance_server.conf
```

- **HotStuff-2**:

```bash
./performance/hs2_performance.sh config/kv_performance_server.conf
```

- **HotStuff-1 with Slotting**:

```bash
./performance/slot_hs1_performance.sh config/kv_performance_server.conf
```

Each benchmark runs for 60 seconds. Results are displayed on-screen and also saved locally.

### Customizing Benchmark Parameters

You can adjust various parameters by editing the config files under `scripts/deploy/config/`, such as `hs1.config` for HotStuff-1.

Key parameters include:

- `clientBatchNum`: Size of each client request batch
- `non_responsive_num`: Number of slow leaders
- `fork_tail_num`: Number of faulty leaders performing tail-forking
- `rollback_num`: Number of faulty leaders performing rollback
- `tpcc_enabled`: Enables the TPC-C benchmark
- `network_delay_num`: Enables network delay injection
- `mean_network_delay`: Average delay (in microseconds) of injected messages