# Composite Key vs. In-Memory Filtering: Performance Benchmark Analysis

## Overview
This document presents performance benchmarks comparing two approaches for querying records by secondary attributes:

- **Current Solution (In-Memory Filtering)**: Retrieves all records using `GetAllValue()` and filters them in memory based on secondary key attributes
- **Proposed Solution (Composite Key Indexing)**: Creates composite keys using a specific encoding format to enable direct record retrieval without scanning

## Benchmark 1: Secondary Key Query Performance
**Objective**: Measure read latency performance when fetching records by secondary attributes using both approaches.

**Test Configuration**:
- Dataset sizes: 1K, 10K, and 100K records
- Query batch size: 10 queries per benchmark run
- Field types: String and Integer secondary keys

**Key Metrics**:
- `items_per_second`: Throughput in queries per second
- `records_scanned_per_query`: Number of records examined per query
- `total_records_scanned`: Total records processed across all queries

```
Running /home/ubuntu/.cache/bazel/_bazel_ubuntu/4daa26ab31ab249b7607bfe58eb14371/execroot/com_resdb_nexres/bazel-out/k8-fastbuild/bin/benchmark/storage/composite_key_benchmark
Run on (8 X 3600.53 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x4)
  L1 Instruction 32 KiB (x4)
  L2 Unified 1024 KiB (x4)
  L3 Unified 36608 KiB (x1)
Load Average: 2.07, 0.97, 0.48
***WARNING*** Library was built as DEBUG. Timings may be affected.
----------------------------------------------------------------------------------------------------------------
Benchmark                                                      Time             CPU   Iterations UserCounters...
----------------------------------------------------------------------------------------------------------------
CompositeKeyBenchmark/StringFieldInMemory/1000/10           43.7 ms         43.7 ms           16 items_per_second=45.7877/s records_scanned_per_query=1k total_records_scanned=32k
CompositeKeyBenchmark/StringFieldInMemory/10000/10           472 ms          472 ms            2 items_per_second=4.23476/s records_scanned_per_query=10k total_records_scanned=40k
CompositeKeyBenchmark/StringFieldInMemory/100000/10         5159 ms         5159 ms            1 items_per_second=0.3877/s records_scanned_per_query=100k total_records_scanned=200k
CompositeKeyBenchmark/StringFieldComposite/1000/10          6.34 ms         6.34 ms          110 items_per_second=315.648/s records_scanned_per_query=1 total_records_scanned=220
CompositeKeyBenchmark/StringFieldComposite/10000/10         73.1 ms         73.1 ms           10 items_per_second=27.3662/s records_scanned_per_query=1 total_records_scanned=20
CompositeKeyBenchmark/StringFieldComposite/100000/10         745 ms          745 ms            1 items_per_second=2.68539/s records_scanned_per_query=1 total_records_scanned=2
CompositeKeyBenchmark/IntegerFieldInMemory/1000/10          43.3 ms         43.3 ms           16 items_per_second=46.1626/s records_scanned_per_query=1k total_records_scanned=32k
CompositeKeyBenchmark/IntegerFieldInMemory/10000/10          471 ms          471 ms            2 items_per_second=4.24955/s records_scanned_per_query=10k total_records_scanned=40k
CompositeKeyBenchmark/IntegerFieldInMemory/100000/10        5132 ms         5131 ms            1 items_per_second=0.389757/s records_scanned_per_query=100k total_records_scanned=200k
CompositeKeyBenchmark/IntegerFieldComposite/1000/10         6.36 ms         6.36 ms          110 items_per_second=314.692/s records_scanned_per_query=1 total_records_scanned=220
CompositeKeyBenchmark/IntegerFieldComposite/10000/10        69.7 ms         69.7 ms           10 items_per_second=28.6814/s records_scanned_per_query=1 total_records_scanned=20
CompositeKeyBenchmark/IntegerFieldComposite/100000/10        732 ms          731 ms            1 items_per_second=2.73417/s records_scanned_per_query=1 total_records_scanned=2
```

## Benchmark 2: Primary Key Retrieval Performance with Composite Key Overhead
**Objective**: Evaluate the performance impact of maintaining composite key indexes on primary key retrieval operations.

**Test Configuration**:
- Dataset sizes: 1K, 10K, 100K, and 1M records
- Composite key ratios: 0%, 25%, 50%, and 100% of records have composite keys
- Field types: String secondary keys (25AF, 50AF, 100AF indicate percentage of records with composite keys)

**Key Metrics**:
- `items_per_second`: Throughput in queries per second
- `composite_keys`: Number of composite key entries maintained
- `total_records`: Total number of records in the dataset

**Performance Analysis**:
- **Baseline Performance**: Primary key retrieval without composite keys
- **Composite Key Impact**: Performance degradation when maintaining composite key indexes
- **Scalability**: Performance trends across different dataset sizes and composite key ratios

- LevelDB

```
Running /home/ubuntu/.cache/bazel/_bazel_ubuntu/4daa26ab31ab249b7607bfe58eb14371/execroot/com_resdb_nexres/bazel-out/k8-fastbuild/bin/benchmark/storage/composite_key_benchmark
Run on (8 X 3604.94 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x4)
  L1 Instruction 32 KiB (x4)
  L2 Unified 1024 KiB (x4)
  L3 Unified 36608 KiB (x1)
Load Average: 0.43, 0.53, 0.49
***WARNING*** Library was built as DEBUG. Timings may be affected.
--------------------------------------------------------------------------------------------------------------------------
Benchmark                                                                Time             CPU   Iterations UserCounters...
--------------------------------------------------------------------------------------------------------------------------
CompositeKeyBenchmark/PrimaryKeyOnly/1000                             18.8 ms         18.8 ms           37 composite_keys=0 items_per_second=53.0803k/s total_records=1k
CompositeKeyBenchmark/PrimaryKeyOnly/10000                             206 ms          206 ms            3 composite_keys=0 items_per_second=48.4508k/s total_records=10k
CompositeKeyBenchmark/PrimaryKeyOnly/100000                           2231 ms         2230 ms            1 composite_keys=0 items_per_second=44.8366k/s total_records=100k
CompositeKeyBenchmark/PrimaryKeyOnly/1000000                         29095 ms        29093 ms            1 composite_keys=0 items_per_second=34.3726k/s total_records=1M
CompositeKeyBenchmark/PrimaryKeyWithCompositeKeys_25AF/1000           19.1 ms         19.1 ms           37 composite_keys=1k items_per_second=52.4668k/s total_records=1k
CompositeKeyBenchmark/PrimaryKeyWithCompositeKeys_25AF/10000           209 ms          209 ms            3 composite_keys=10k items_per_second=47.8541k/s total_records=10k
CompositeKeyBenchmark/PrimaryKeyWithCompositeKeys_25AF/100000         2233 ms         2233 ms            1 composite_keys=100k items_per_second=44.7865k/s total_records=100k
CompositeKeyBenchmark/PrimaryKeyWithCompositeKeys_25AF/1000000       34021 ms        34019 ms            1 composite_keys=1M items_per_second=29.3955k/s total_records=1M
CompositeKeyBenchmark/PrimaryKeyWithCompositeKeys_50AF/1000           20.7 ms         20.7 ms           36 composite_keys=1k items_per_second=48.3132k/s total_records=1k
CompositeKeyBenchmark/PrimaryKeyWithCompositeKeys_50AF/10000           209 ms          209 ms            3 composite_keys=10k items_per_second=47.7968k/s total_records=10k
CompositeKeyBenchmark/PrimaryKeyWithCompositeKeys_50AF/100000         2335 ms         2335 ms            1 composite_keys=100k items_per_second=42.8258k/s total_records=100k
CompositeKeyBenchmark/PrimaryKeyWithCompositeKeys_50AF/1000000       30012 ms        30010 ms            1 composite_keys=1M items_per_second=33.322k/s total_records=1M
CompositeKeyBenchmark/PrimaryKeyWithCompositeKeys_100AF/1000          21.8 ms         21.8 ms           35 composite_keys=1k items_per_second=45.9682k/s total_records=1k
CompositeKeyBenchmark/PrimaryKeyWithCompositeKeys_100AF/10000          227 ms          227 ms            3 composite_keys=10k items_per_second=44.0984k/s total_records=10k
CompositeKeyBenchmark/PrimaryKeyWithCompositeKeys_100AF/100000        2370 ms         2370 ms            1 composite_keys=100k items_per_second=42.2014k/s total_records=100k
CompositeKeyBenchmark/PrimaryKeyWithCompositeKeys_100AF/1000000      97125 ms        96949 ms            1 composite_keys=1M items_per_second=10.3147k/s total_records=1M
```

# Latency Comparison: Sequential Reads (time in ms)

| Records | Primary Only | 25% Composite Write Amplification | 50% Composite Write Amplification | 100% Composite Write Amplification |
|---------|-------------|---------------|---------------|----------------|
| 1K      | 18.8        | 19.1          | 20.7          | 21.8          |
| 10K     | 206         | 209           | 209           | 227           |
| 100K    | 2231        | 2233          | 2335          | 2370          |
| 1M      | 29095       | 34021         | 30012         | 97125         |

# Latency Comparison: Sequential Reads (time in ms)

| Records | Primary Only | 25% Composite (Overhead) | 50% Composite (Overhead) | 100% Composite (Overhead) |
|---------|-------------|-------------------------|-------------------------|--------------------------|
| 1K      | 18.8        | 19.1 (+1.6%)           | 20.7 (+10.1%)          | 21.8 (+16.0%)           |
| 10K     | 206         | 209 (+1.5%)            | 209 (+1.5%)            | 227 (+10.2%)            |
| 100K    | 2231        | 2233 (+0.1%)           | 2335 (+4.7%)           | 2370 (+6.2%)            |
| 1M      | 29095       | 34021 (+16.9%)         | 30012 (+3.2%)          | 97125 (+233.8%)         |

- RocksDB (BlobDB Enabled)
```
INFO: Running command line: bazel-bin/benchmark/storage/composite_key_benchmark '--benchmark_filter=PrimaryKey*' '--storage_type=rocksdb'
2025-09-03T19:46:17+00:00
Running /home/ubuntu/.cache/bazel/_bazel_ubuntu/4daa26ab31ab249b7607bfe58eb14371/execroot/com_resdb_nexres/bazel-out/k8-fastbuild/bin/benchmark/storage/composite_key_benchmark
Run on (8 X 3596.06 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x4)
  L1 Instruction 32 KiB (x4)
  L2 Unified 1024 KiB (x4)
  L3 Unified 36608 KiB (x1)
Load Average: 0.26, 0.39, 0.56
***WARNING*** Library was built as DEBUG. Timings may be affected.
--------------------------------------------------------------------------------------------------------------------------
Benchmark                                                                Time             CPU   Iterations UserCounters...
--------------------------------------------------------------------------------------------------------------------------
CompositeKeyBenchmark/PrimaryKeyOnly/1000                             16.1 ms         16.1 ms           44 composite_keys=0 items_per_second=61.9675k/s total_records=1k
CompositeKeyBenchmark/PrimaryKeyOnly/10000                             170 ms          170 ms            4 composite_keys=0 items_per_second=58.6594k/s total_records=10k
CompositeKeyBenchmark/PrimaryKeyOnly/100000                           1780 ms         1780 ms            1 composite_keys=0 items_per_second=56.1806k/s total_records=100k
CompositeKeyBenchmark/PrimaryKeyOnly/1000000                         19080 ms        19079 ms            1 composite_keys=0 items_per_second=52.4128k/s total_records=1M
CompositeKeyBenchmark/PrimaryKeyWithCompositeKeys_25AF/1000           16.7 ms         16.7 ms           44 composite_keys=1k items_per_second=60.0398k/s total_records=1k
CompositeKeyBenchmark/PrimaryKeyWithCompositeKeys_25AF/10000           172 ms          172 ms            4 composite_keys=10k items_per_second=58.2706k/s total_records=10k
CompositeKeyBenchmark/PrimaryKeyWithCompositeKeys_25AF/100000         1864 ms         1864 ms            1 composite_keys=100k items_per_second=53.6553k/s total_records=100k
CompositeKeyBenchmark/PrimaryKeyWithCompositeKeys_25AF/1000000       20900 ms        20899 ms            1 composite_keys=1M items_per_second=47.8487k/s total_records=1M
CompositeKeyBenchmark/PrimaryKeyWithCompositeKeys_50AF/1000           16.2 ms         16.2 ms           43 composite_keys=1k items_per_second=61.705k/s total_records=1k
CompositeKeyBenchmark/PrimaryKeyWithCompositeKeys_50AF/10000           173 ms          173 ms            4 composite_keys=10k items_per_second=57.7472k/s total_records=10k
CompositeKeyBenchmark/PrimaryKeyWithCompositeKeys_50AF/100000         1823 ms         1823 ms            1 composite_keys=100k items_per_second=54.8692k/s total_records=100k
CompositeKeyBenchmark/PrimaryKeyWithCompositeKeys_50AF/1000000       23071 ms        23070 ms            1 composite_keys=1M items_per_second=43.3468k/s total_records=1M
CompositeKeyBenchmark/PrimaryKeyWithCompositeKeys_100AF/1000          16.3 ms         16.3 ms           41 composite_keys=1k items_per_second=61.4066k/s total_records=1k
CompositeKeyBenchmark/PrimaryKeyWithCompositeKeys_100AF/10000          171 ms          171 ms            4 composite_keys=10k items_per_second=58.3699k/s total_records=10k
CompositeKeyBenchmark/PrimaryKeyWithCompositeKeys_100AF/100000        1853 ms         1852 ms            1 composite_keys=100k items_per_second=53.9821k/s total_records=100k
CompositeKeyBenchmark/PrimaryKeyWithCompositeKeys_100AF/1000000      21148 ms        21147 ms            1 composite_keys=1M items_per_second=47.2885k/s total_records=1M
```
# Latency Comparison: Sequential Reads (time in ms)

| Records | Primary Only | 25% Composite Write Amplification | 50% Composite Write Amplification | 100% Composite Write Amplification |
|---------|-------------|---------------|---------------|----------------|
| 1K      | 16.1        | 16.7          | 16.2          | 16.3          |
| 10K     | 170         | 172           | 173           | 171           |
| 100K    | 1780        | 1864          | 1823          | 1853          |
| 1M      | 19080       | 20900         | 23071         | 21148         |

# Latency Comparison: RocksDB with BlobDB, Sequential Reads (time in ms) with Overhead

| Records | Primary Only | 25% Composite (Overhead) | 50% Composite (Overhead) | 100% Composite (Overhead) |
|---------|-------------|-------------------------|-------------------------|--------------------------|
| 1K      | 16.1        | 16.7 (+3.7%)           | 16.2 (+0.6%)           | 16.3 (+1.2%)            |
| 10K     | 170         | 172 (+1.2%)            | 173 (+1.8%)            | 171 (+0.6%)             |
| 100K    | 1780        | 1864 (+4.7%)           | 1823 (+2.4%)           | 1853 (+4.1%)            |
| 1M      | 19080       | 20900 (+9.5%)          | 23071 (+20.9%)         | 21148 (+10.8%)          |

- RocksDB (PrefixCapped search with bloom filters)
```
INFO: Running command line: bazel-bin/benchmark/storage/composite_key_benchmark '--benchmark_filter=PrimaryKey*' '--storage_type=rocksdb'
2025-09-03T20:06:11+00:00
Running /home/ubuntu/.cache/bazel/_bazel_ubuntu/4daa26ab31ab249b7607bfe58eb14371/execroot/com_resdb_nexres/bazel-out/k8-fastbuild/bin/benchmark/storage/composite_key_benchmark
Run on (8 X 3599.71 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x4)
  L1 Instruction 32 KiB (x4)
  L2 Unified 1024 KiB (x4)
  L3 Unified 36608 KiB (x1)
Load Average: 0.41, 0.48, 0.60
***WARNING*** Library was built as DEBUG. Timings may be affected.
--------------------------------------------------------------------------------------------------------------------------
Benchmark                                                                Time             CPU   Iterations UserCounters...
--------------------------------------------------------------------------------------------------------------------------
CompositeKeyBenchmark/PrimaryKeyOnly/1000                             16.4 ms         16.4 ms           43 composite_keys=0 items_per_second=60.9054k/s total_records=1k
CompositeKeyBenchmark/PrimaryKeyOnly/10000                             171 ms          171 ms            4 composite_keys=0 items_per_second=58.4813k/s total_records=10k
CompositeKeyBenchmark/PrimaryKeyOnly/100000                           1800 ms         1800 ms            1 composite_keys=0 items_per_second=55.5449k/s total_records=100k
CompositeKeyBenchmark/PrimaryKeyOnly/1000000                         19564 ms        19563 ms            1 composite_keys=0 items_per_second=51.1175k/s total_records=1M
CompositeKeyBenchmark/PrimaryKeyWithCompositeKeys_25AF/1000           16.5 ms         16.5 ms           43 composite_keys=1k items_per_second=60.7669k/s total_records=1k
CompositeKeyBenchmark/PrimaryKeyWithCompositeKeys_25AF/10000           170 ms          170 ms            4 composite_keys=10k items_per_second=58.8057k/s total_records=10k
CompositeKeyBenchmark/PrimaryKeyWithCompositeKeys_25AF/100000         1792 ms         1792 ms            1 composite_keys=100k items_per_second=55.8123k/s total_records=100k
CompositeKeyBenchmark/PrimaryKeyWithCompositeKeys_25AF/1000000       21907 ms        21905 ms            1 composite_keys=1M items_per_second=45.6507k/s total_records=1M
CompositeKeyBenchmark/PrimaryKeyWithCompositeKeys_50AF/1000           16.3 ms         16.3 ms           43 composite_keys=1k items_per_second=61.4517k/s total_records=1k
CompositeKeyBenchmark/PrimaryKeyWithCompositeKeys_50AF/10000           174 ms          174 ms            4 composite_keys=10k items_per_second=57.3433k/s total_records=10k
CompositeKeyBenchmark/PrimaryKeyWithCompositeKeys_50AF/100000         1830 ms         1830 ms            1 composite_keys=100k items_per_second=54.6481k/s total_records=100k
CompositeKeyBenchmark/PrimaryKeyWithCompositeKeys_50AF/1000000       21594 ms        21593 ms            1 composite_keys=1M items_per_second=46.3111k/s total_records=1M
CompositeKeyBenchmark/PrimaryKeyWithCompositeKeys_100AF/1000          16.3 ms         16.3 ms           42 composite_keys=1k items_per_second=61.4314k/s total_records=1k
CompositeKeyBenchmark/PrimaryKeyWithCompositeKeys_100AF/10000          174 ms          174 ms            4 composite_keys=10k items_per_second=57.418k/s total_records=10k
CompositeKeyBenchmark/PrimaryKeyWithCompositeKeys_100AF/100000        1841 ms         1841 ms            1 composite_keys=100k items_per_second=54.325k/s total_records=100k
CompositeKeyBenchmark/PrimaryKeyWithCompositeKeys_100AF/1000000      21040 ms        21039 ms            1 composite_keys=1M items_per_second=47.5313k/s total_records=1M
```

# Latency Comparison: RocksDB with Prefix Optimization, Sequential Reads (time in ms)

| Records | Primary Only | 25% Composite Write Amplification | 50% Composite Write Amplification | 100% Composite Write Amplification |
|---------|-------------|---------------|---------------|----------------|
| 1K      | 16.4        | 16.5          | 16.3          | 16.3          |
| 10K     | 171         | 170           | 174           | 174           |
| 100K    | 1800        | 1792          | 1830          | 1841          |
| 1M      | 19564       | 21907         | 21594         | 21040         |

# Latency Comparison: RocksDB with Prefix Optimization, Sequential Reads (time in ms) with Overhead

| Records | Primary Only | 25% Composite (Overhead) | 50% Composite (Overhead) | 100% Composite (Overhead) |
|---------|-------------|-------------------------|-------------------------|--------------------------|
| 1K      | 16.4        | 16.5 (+0.6%)           | 16.3 (-0.6%)           | 16.3 (-0.6%)            |
| 10K     | 171         | 170 (-0.6%)            | 174 (+1.8%)            | 174 (+1.8%)             |
| 100K    | 1800        | 1792 (-0.4%)           | 1830 (+1.7%)           | 1841 (+2.3%)            |
| 1M      | 19564       | 21907 (+12.0%)         | 21594 (+10.4%)         | 21040 (+7.5%)           |

## Benchmark 3: RocksDB vs LevelDB
```
INFO: Running command line: bazel-bin/benchmark/storage/composite_key_benchmark '--benchmark_filter=IntegerFieldComposite' '--storage_type=rocksdb'
2025-09-03T20:19:25+00:00
Running /home/ubuntu/.cache/bazel/_bazel_ubuntu/4daa26ab31ab249b7607bfe58eb14371/execroot/com_resdb_nexres/bazel-out/k8-fastbuild/bin/benchmark/storage/composite_key_benchmark
Run on (8 X 3600.21 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x4)
  L1 Instruction 32 KiB (x4)
  L2 Unified 1024 KiB (x4)
  L3 Unified 36608 KiB (x1)
Load Average: 0.43, 0.40, 0.53
***WARNING*** Library was built as DEBUG. Timings may be affected.
--------------------------------------------------------------------------------------------------------------
Benchmark                                                    Time             CPU   Iterations UserCounters...
--------------------------------------------------------------------------------------------------------------
CompositeKeyBenchmark/IntegerFieldComposite/1000          4.30 ms         4.30 ms          168 items_per_second=465.361/s records_scanned_per_query=1 total_records_scanned=336
CompositeKeyBenchmark/IntegerFieldComposite/10000         44.5 ms         44.5 ms           16 items_per_second=44.937/s records_scanned_per_query=1 total_records_scanned=32
CompositeKeyBenchmark/IntegerFieldComposite/100000         490 ms          490 ms            2 items_per_second=4.08414/s records_scanned_per_query=1 total_records_scanned=4
CompositeKeyBenchmark/IntegerFieldComposite/1000000       6250 ms         6250 ms            1 items_per_second=0.320023/s records_scanned_per_query=1 total_records_scanned=2

----------------------------------------------------------------------------------------------------------------------------------------------------------------

ubuntu@ip-172-31-38-52:/opt/resilientdb$ bazel run //benchmark/storage:composite_key_benchmark -- --benchmark_filter="IntegerFieldComposite" --storage_type="leveldb"
INFO: Analyzed target //benchmark/storage:composite_key_benchmark (0 packages loaded, 0 targets configured).
INFO: Found 1 target...
Target //benchmark/storage:composite_key_benchmark up-to-date:
  bazel-bin/benchmark/storage/composite_key_benchmark
INFO: Elapsed time: 0.128s, Critical Path: 0.00s
INFO: 1 process: 1 internal.
INFO: Build completed successfully, 1 total action
INFO: Running command line: bazel-bin/benchmark/storage/composite_key_benchmark '--benchmark_filter=IntegerFieldComposite' '--storage_type=leveldb'
2025-09-03T20:21:25+00:00
Running /home/ubuntu/.cache/bazel/_bazel_ubuntu/4daa26ab31ab249b7607bfe58eb14371/execroot/com_resdb_nexres/bazel-out/k8-fastbuild/bin/benchmark/storage/composite_key_benchmark
Run on (8 X 3602.54 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x4)
  L1 Instruction 32 KiB (x4)
  L2 Unified 1024 KiB (x4)
  L3 Unified 36608 KiB (x1)
Load Average: 0.94, 0.62, 0.60
***WARNING*** Library was built as DEBUG. Timings may be affected.
--------------------------------------------------------------------------------------------------------------
Benchmark                                                    Time             CPU   Iterations UserCounters...
--------------------------------------------------------------------------------------------------------------
CompositeKeyBenchmark/IntegerFieldComposite/1000          6.36 ms         6.36 ms          110 items_per_second=314.239/s records_scanned_per_query=1 total_records_scanned=220
CompositeKeyBenchmark/IntegerFieldComposite/10000         70.5 ms         70.5 ms           10 items_per_second=28.3702/s records_scanned_per_query=1 total_records_scanned=20
CompositeKeyBenchmark/IntegerFieldComposite/100000         741 ms          741 ms            1 items_per_second=2.69975/s records_scanned_per_query=1 total_records_scanned=2
CompositeKeyBenchmark/IntegerFieldComposite/1000000      10160 ms        10159 ms            1 items_per_second=0.196871/s records_scanned_per_query=1 total_records_scanned=2
```