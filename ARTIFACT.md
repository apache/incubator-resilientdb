# Cassandra Artifact

This branch contains the implementation and evaluation material for
"Cassandra: Consensus with Partial Progress via Robust Partitionable View
Synchronization." Cassandra is implemented in C++ within Apache ResilientDB.
The artifact includes the protocol source, benchmark launchers, the recorded
Figure 12 traces, and the script that regenerates Figure 12(a)-(h).

The paper is available at <https://arxiv.org/abs/2607.02856>.

## Artifact contents

| Material | Location |
| --- | --- |
| Cassandra consensus implementation | `platform/consensus/ordering/cassandra/` |
| Cassandra benchmark server and client | `benchmark/protocols/cassandra/` |
| Distributed deployment framework | `scripts/deploy/` |
| Cassandra deployment launcher | `scripts/deploy/performance/cassandra_performance.sh` |
| Baseline deployment launchers | `scripts/deploy/performance/{tusk,autobahn,pbft,hs,spotless,rcc}_performance.sh` |
| Figure 12 plotting code | `artifact/figure12/plot_figure_12_evaluation.py` |
| Figure 12 recorded data | `artifact/figure12/data/` |

The files under `artifact/figure12/data/recover_data/` are the recorded
two-second throughput and latency traces used by the timeline panels. The
scalability values are in `artifact/figure12/data/scalability_data.tex`.

## 1. Clone and build

The build has been exercised on Ubuntu 20.04 or later with Bazel 6.3.2.
Installation requires `sudo` because `INSTALL.sh` installs system packages.

```bash
git clone --branch cassandra --single-branch \
  https://github.com/apache/incubator-resilientdb.git
cd incubator-resilientdb
./INSTALL.sh
bazel build //platform/consensus/ordering/cassandra/...
bazel build //benchmark/protocols/cassandra:kv_server_performance
```

The last command should produce
`bazel-bin/benchmark/protocols/cassandra/kv_server_performance`.

## 2. Configure a distributed run

The deployment scripts assume passwordless SSH from the coordinator to every
host. Start from the supplied example:

```bash
cp scripts/deploy/config/cassandra_cluster.example.conf \
  scripts/deploy/config/cassandra_cluster.conf
cp scripts/deploy/config/key_example.conf scripts/deploy/config/key.conf
```

Edit both local files:

- `iplist` lists replica hosts first and client hosts last.
- `client_num` is the number of client hosts at the end of `iplist`.
- `ssh_user` and `remote_home` identify the remote login account.
- `benchmark_seconds` controls the measurement duration.
- `scripts/deploy/config/key.conf` sets `key` to the private SSH key path.

Do not commit private keys or cloud credentials. The coordinator and remote
hosts must be able to communicate on the generated replica ports, which start
at TCP port 17001.

Run Cassandra from the deployment directory:

```bash
cd scripts/deploy
./performance/cassandra_performance.sh config/cassandra_cluster.conf
```

The launcher builds the selected target, generates replica keys and configs,
copies them to the hosts, starts the benchmark, retrieves the logs, and writes
the aggregate result to `scripts/deploy/results.log`.

The corresponding baseline commands are:

```bash
./performance/tusk_performance.sh config/cassandra_cluster.conf
./performance/autobahn_performance.sh config/cassandra_cluster.conf
./performance/pbft_performance.sh config/cassandra_cluster.conf
./performance/hs_performance.sh config/cassandra_cluster.conf
./performance/spotless_performance.sh config/cassandra_cluster.conf
./performance/rcc_performance.sh config/cassandra_cluster.conf
```

Run one protocol at a time. The paper averages three independent runs for each
configuration.

## 3. Figure 12 experiment matrix

Use the launchers above for each protocol and apply the network schedule after
the clients print `start benchmark`. The recorded data are mapped as follows:

| Panel | Experiment | Recorded data | Schedule |
| --- | --- | --- | --- |
| (a) | Stable-network scalability at 16, 32, 48, 64, and 104 replicas | `data/scalability_data.tex` | No injected fault |
| (b) | `0 -> f -> 0` partition | `data/recover_data/f_fail/` | Partition from 24 s to 44 s |
| (c) | `f -> f+1 -> f` partition | `data/recover_data/f-f+1-f fail/` | Add one partitioned replica from 24 s to 32 s |
| (d) | `f -> n/2 -> f` partition | `data/recover_data/f-n2-f fail/` | Expand to a balanced partition from 24 s to 32 s |
| (e) | `f` delayed replicas | `data/recover_data/delay/` | Add 50 ms delay from 24 s to 44 s |
| (f) | Tail-forking attack | `data/recover_data/ncf/` | Enable non-consecutive faulty leader positions from 24 s to 44 s |
| (g) | Cassandra ablation | `data/recover_data/f_fail/` and `data/recover_data/f-f+1-f fail/` | Reuse the schedules in (b) and (c) |
| (h) | Four-region partition | `data/recover_data/geo/` | `0 -> f -> f+1 -> f -> 0` at 14, 34, 44, and 66 s |

For panel (a), change the replica count in the cluster file and keep the same
request size, batching configuration, and offered load across protocols. For
panel (h), the paper uses 31 replicas in Northern Virginia, Sao Paulo,
Frankfurt, and Singapore, with regional group sizes `8/8/8/7`.

### Network fault injection

The partition experiments use symmetric link failures. On both sides of each
partition boundary, block only the peer's consensus traffic so that SSH remains
available. For example, for a generated port range of 17001-17150:

```bash
sudo iptables -I INPUT  -p tcp --dport 17001:17150 -s PEER_IP -j DROP
sudo iptables -I OUTPUT -p tcp --dport 17001:17150 -d PEER_IP -j DROP
```

Remove the same rules at the end of the interval by replacing `-I` with `-D`.
Apply the rules in both directions for every cross-component peer pair. Verify
the active rules with `sudo iptables -S` before collecting a run.

For panel (e), add and later remove a 50 ms delay on each selected faulty host:

```bash
sudo tc qdisc replace dev eth0 root netem delay 50ms
sudo tc qdisc del dev eth0 root
```

Replace `eth0` with the interface carrying replica traffic. The tail-forking
experiment is a protocol-level Byzantine schedule rather than a network drop:
the `f` faulty replicas are placed at non-consecutive round/leader positions so
that they create short forks or unfinished tails. The branch contains the
resulting traces, but the experiment-specific tail-forking scheduler is not
exposed as a standalone command-line tool.

The ablation traces correspond to these incremental variants:

- `No-Diss-No-Spec`: dissemination and speculative execution disabled.
- `No-Spec`: dissemination enabled and speculative execution disabled.
- `Cassandra`: both mechanisms enabled.

The current branch does not expose the two ablation switches as stable
command-line options. To reproduce the submitted panel exactly, the plotting
script also applies the documented deterministic transformations in
`_jitter_ablation_trace` and `_build_partition_zero_burst_trace` to the
available source traces. Independent raw run logs for every ablation variant
are not included.

## 4. Regenerate Figure 12

Figure regeneration does not require AWS resources or a C++ build. Use Python
3.12 to match the submitted figure's plotting environment.

```bash
python3.12 -m venv .venv-figure12
source .venv-figure12/bin/activate
python3 -m pip install -r artifact/figure12/requirements.txt
python3 artifact/figure12/plot_figure_12_evaluation.py
```

The command writes:

- `artifact/figure12/output/figure_12_evaluation.pdf`
- `artifact/figure12/output/figure_12_evaluation.png`
- `artifact/figure12/output/all.pdf`

As a quick data check, panel (a) should report Cassandra at approximately
477.1K TPS with 104 replicas. The generated PDF contains all eight panels in
the order listed above and embeds TrueType fonts rather than Type 3 fonts.

## Scope and reuse notes

- The supplied traces and deterministic plotting steps permit exact
  regeneration and inspection of Figure 12.
- Re-running the distributed experiments requires machines, SSH access, and
  external network-fault scheduling at the times listed above.
- Disseminated or PoA-backed speculative work in weak-quorum intervals is not
  final committed throughput; the paper and plots preserve this distinction.
- Cloud costs and resource cleanup are the responsibility of the evaluator.

Questions and reproducibility reports can be filed through this repository's
GitHub issue tracker. Source code is distributed under the repository's Apache
License 2.0; see `LICENSE` and `NOTICE`.
