This program aims to stress test the KV store using the configuration below. We try to find at roughly what value size (MB) does the KV store start experiencing problems e.g extremely long set time or infinite wait time. For the setup below we experienced problems around the 150MB - 200MB mark. Note we are only testing the set function. 

Note, all commands assume you're in the indexers-ECS265-Fall2025 directory, basically the starting directory to the repo. Also have the kv service running correctly in the first place.  To run this, first go to service\tools\kv\api_tools\kv_service_tools.cpp and find all comments with #SIZE TEST and uncomment those blocks. You'll need to rerun INSTALL.sh. If you're getting problems for this try running bazel build service/tools/kv/api_tools/kv_service_tools.
Once the KV is up and running, use the Test File Generate Command to create a random test file of specific size. Afterwards you may use any of the benchmark commands. 

# KV Store Stress Test

This program is designed to stress test the KV store's `set` function. The goal is to identify the value size (in MB) at which the KV store begins to experience performance degradation (e.g., extremely long set times or infinite wait times).

> **Findings:** Using the configuration below, we experienced problems around the **150MB - 200MB** mark.

## Prerequisites & Setup

**Important:** All commands assume you are in the root directory of the repo: `indexers-ECS265-Fall2025`. You must also have the KV service running.

1. **Modify Source:** Navigate to `service/tools/kv/api_tools/kv_service_tools.cpp`.
2. **Enable Test:** Find all comments marked with `#SIZE TEST` and uncomment those code blocks.
3. **Build:** Rerun the install script:
   ```bash
   ./INSTALL.sh
	 ```
4. If you run into bazel problems
   ```
	 bazel build service/tools/kv/api_tools/kv_service_tools
	 ```
## Example Configuration:
 8GB RAM Shell
 Standard 5 replica config from `./service/tools/kv/server_tools/start_kv_service.sh`

### SET Command: 
	bazel-bin/service/tools/kv/api_tools/kv_service_tools \
		--config service/tools/config/interface/service.config \
		--cmd set_with_version \
		--key key1 \
		--version 0 \
		--value_path hnsw-test/index_test/FILE
	EX: bazel-bin/service/tools/kv/api_tools/kv_service_tools \
		--config service/tools/config/interface/service.config \
		--cmd set_with_version \
		--key key1 \
		--version 0 \
		--value_path hnsw-test/index_test/size.txt

### Benchmark command:
	python3 hnsw-test/index_test/benchmark_set.py KEY FILE
	EX: python3 hnsw-test/index_test/benchmark_set.py key1 hnsw-test/index_test/val_50mb.txt

### Multiple Benchmark command:
	python3 hnsw-test/index_test/multi_benchmarks.py KEY FILE TEST_AMOUNT(INT)
	EX: python3 hnsw-test/index_test/multi_benchmarks.py key1 hnsw-test/index_test/val_200mb.txt 5

### Test File Generate command:
	python3 hnsw-test/index_test/gen_files.py SIZE_IN_MB
	EX: python3 hnsw-test/index_test/gen_files.py 100
