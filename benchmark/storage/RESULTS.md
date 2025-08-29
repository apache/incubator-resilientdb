# Prefix Search without bloom filter (Debug build)

```
2025-08-28T01:46:48+00:00
Running /home/ubuntu/.cache/bazel/_bazel_ubuntu/4daa26ab31ab249b7607bfe58eb14371/execroot/com_resdb_nexres/bazel-out/k8-fastbuild/bin/benchmark/storage/composite_key_benchmark
Run on (8 X 3537.35 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x4)
  L1 Instruction 32 KiB (x4)
  L2 Unified 1024 KiB (x4)
  L3 Unified 36608 KiB (x1)
Load Average: 0.67, 0.92, 0.94
***WARNING*** Library was built as DEBUG. Timings may be affected.
----------------------------------------------------------------------------------------------------------------
Benchmark                                                      Time             CPU   Iterations UserCounters...
----------------------------------------------------------------------------------------------------------------
CompositeKeyBenchmark/StringFieldInMemory/1000/10           38.3 ms         38.3 ms           18 items_per_second=52.2105k/s
CompositeKeyBenchmark/StringFieldInMemory/10000/10           421 ms          421 ms            2 items_per_second=47.5181k/s
CompositeKeyBenchmark/StringFieldInMemory/100000/10         4534 ms         4534 ms            1 items_per_second=44.1125k/s
CompositeKeyBenchmark/StringFieldComposite/1000/10          6.37 ms         6.37 ms          102 items_per_second=314.061/s
CompositeKeyBenchmark/StringFieldComposite/10000/10         73.9 ms         73.9 ms           10 items_per_second=27.0687/s
CompositeKeyBenchmark/StringFieldComposite/100000/10         741 ms          741 ms            1 items_per_second=2.69846/s
CompositeKeyBenchmark/IntegerFieldInMemory/1000/10          37.9 ms         37.9 ms           19 items_per_second=52.8361k/s
CompositeKeyBenchmark/IntegerFieldInMemory/10000/10          415 ms          415 ms            2 items_per_second=48.2257k/s
CompositeKeyBenchmark/IntegerFieldInMemory/100000/10        4540 ms         4540 ms            1 items_per_second=44.0566k/s
CompositeKeyBenchmark/IntegerFieldComposite/1000/10         6.35 ms         6.35 ms          110 items_per_second=315.142/s
CompositeKeyBenchmark/IntegerFieldComposite/10000/10        70.6 ms         70.6 ms           10 items_per_second=28.3272/s
CompositeKeyBenchmark/IntegerFieldComposite/100000/10        738 ms          738 ms            1 items_per_second=2.71079/s
```

-----------------------------------------------------------------------------------------------------------------------------------------------------------

# Prefix Search with bloom filter (Debug build)
```
Running /home/ubuntu/.cache/bazel/_bazel_ubuntu/4daa26ab31ab249b7607bfe58eb14371/execroot/com_resdb_nexres/bazel-out/k8-fastbuild/bin/benchmark/storage/composite_key_benchmark
Run on (8 X 3577.75 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x4)
  L1 Instruction 32 KiB (x4)
  L2 Unified 1024 KiB (x4)
  L3 Unified 36608 KiB (x1)
Load Average: 0.93, 0.69, 0.49
***WARNING*** Library was built as DEBUG. Timings may be affected.
----------------------------------------------------------------------------------------------------------------
Benchmark                                                      Time             CPU   Iterations UserCounters...
----------------------------------------------------------------------------------------------------------------
CompositeKeyBenchmark/StringFieldInMemory/1000/10           38.2 ms         38.2 ms           18 items_per_second=52.3364k/s
CompositeKeyBenchmark/StringFieldInMemory/10000/10           417 ms          417 ms            2 items_per_second=47.9942k/s
CompositeKeyBenchmark/StringFieldInMemory/100000/10         4539 ms         4539 ms            1 items_per_second=44.0652k/s
CompositeKeyBenchmark/StringFieldComposite/1000/10          6.54 ms         6.54 ms           98 items_per_second=305.794/s
CompositeKeyBenchmark/StringFieldComposite/10000/10         72.0 ms         72.0 ms           10 items_per_second=27.7821/s
CompositeKeyBenchmark/StringFieldComposite/100000/10         763 ms          763 ms            1 items_per_second=2.62157/s
CompositeKeyBenchmark/IntegerFieldInMemory/1000/10          37.9 ms         37.9 ms           17 items_per_second=52.7949k/s
CompositeKeyBenchmark/IntegerFieldInMemory/10000/10          417 ms          417 ms            2 items_per_second=47.9945k/s
CompositeKeyBenchmark/IntegerFieldInMemory/100000/10        4503 ms         4502 ms            1 items_per_second=44.4212k/s
CompositeKeyBenchmark/IntegerFieldComposite/1000/10         6.49 ms         6.49 ms          107 items_per_second=308.003/s
CompositeKeyBenchmark/IntegerFieldComposite/10000/10        71.3 ms         71.3 ms           10 items_per_second=28.0354/s
CompositeKeyBenchmark/IntegerFieldComposite/100000/10        760 ms          760 ms            1 items_per_second=2.63277/s
```