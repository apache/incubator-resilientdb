#!/usr/bin/env python3
"""
Adversarial Network Experiment Scheduler for ResilientDB BFT protocols.

Reads a JSON scenario file describing time-ordered adversarial events
and applies them to the deployed cluster via SSH.

Supported actions:
  - set_delay: Change network delay/jitter on all or specific nodes via tc netem
  - crash: Kill replica processes on specified nodes
  - partition: Create network partition between two groups via iptables
  - heal: Remove all iptables partition rules
  - set_delay_nodes: Set per-node delays (for targeted attacks)

Usage:
  python3 adversarial_scheduler.py --scenario scenarios/gst_ramp.json \
      --ip-conf ../config/kv_performance_server_16.conf \
      --key ~/.ssh/key.pem --server-bin kv_server_performance \
      [--ssh-port 2222] [--dry-run]
"""

import argparse
import json
import subprocess
import sys
import time
import threading
import os
import re


def parse_ip_conf(conf_path):
    """Parse the IP list from a .conf file (bash array format)."""
    ips = []
    with open(conf_path) as f:
        for line in f:
            line = line.strip()
            # Match IP addresses
            m = re.match(r'^(\d+\.\d+\.\d+\.\d+)\s*$', line)
            if m:
                ips.append(m.group(1))
    return ips


def parse_client_num(conf_path):
    """Parse client_num from the .conf file."""
    with open(conf_path) as f:
        for line in f:
            if 'client_num' in line:
                m = re.search(r'client_num\s*=\s*(\d+)', line)
                if m:
                    return int(m.group(1))
    return 1


def ssh_cmd(ip, cmd, key, port=2222, dry_run=False, user="root"):
    """Execute a command on a remote host via SSH."""
    if dry_run:
        print(f"  [DRY-RUN] {ip}: {cmd}")
        return
    try:
        # Use list form (no shell=True) to avoid local shell expanding $() and $VAR
        # inside the remote command string.
        argv = [
            "ssh",
            "-o", "StrictHostKeyChecking=no",
            "-o", "LogLevel=ERROR",
            "-o", "UserKnownHostsFile=/dev/null",
            "-o", "ServerAliveInterval=60",
            "-p", str(port),
            "-i", key,
            "-n",
            "-o", "BatchMode=yes",
            f"{user}@{ip}",
            cmd,
        ]
        result = subprocess.run(argv, capture_output=True, text=True, timeout=30)
        if result.returncode != 0 and result.stderr:
            print(f"  [WARN] {ip}: {result.stderr.strip()}", file=sys.stderr)
    except subprocess.TimeoutExpired:
        print(f"  [WARN] {ip}: SSH command timed out", file=sys.stderr)


def run_on_all(ips, cmd, key, port=2222, dry_run=False, user="root"):
    """Run a command on all IPs in parallel."""
    threads = []
    for ip in ips:
        t = threading.Thread(target=ssh_cmd, args=(ip, cmd, key, port, dry_run, user))
        t.start()
        threads.append(t)
    for t in threads:
        t.join()


def run_on_nodes(ips, node_indices, cmd, key, port=2222, dry_run=False, user="root"):
    """Run a command on specific node indices (1-based)."""
    threads = []
    for idx in node_indices:
        if 1 <= idx <= len(ips):
            ip = ips[idx - 1]
            t = threading.Thread(target=ssh_cmd, args=(ip, cmd, key, port, dry_run, user))
            t.start()
            threads.append(t)
    for t in threads:
        t.join()


def action_set_delay(ips, phase, key, port, dry_run, tc_user="Shaokang"):
    """Set uniform network delay on all nodes via host tc (sudo required)."""
    delay_ms = phase.get("delay_ms", 40)
    jitter_ms = phase.get("jitter_ms", 0)
    dist = phase.get("distribution", "")
    dist_arg = f" distribution {dist}" if dist else ""

    # Auto-detect the outgoing interface, then apply netem delay via sudo
    tc_cmd = (
        f"DEV=$(ip -4 route show default | awk '{{print $5}}' | head -1) && "
        f"sudo tc qdisc change dev $DEV root netem delay {delay_ms}ms {jitter_ms}ms{dist_arg} 2>/dev/null "
        f"|| sudo tc qdisc add dev $DEV root netem delay {delay_ms}ms {jitter_ms}ms{dist_arg}"
    )
    target = phase.get("nodes", None)
    if target:
        print(f"  Setting delay {delay_ms}ms +/- {jitter_ms}ms on nodes {target}")
        run_on_nodes(ips, target, tc_cmd, key, port, dry_run, user=tc_user)
    else:
        print(f"  Setting delay {delay_ms}ms +/- {jitter_ms}ms on ALL nodes")
        run_on_all(ips, tc_cmd, key, port, dry_run, user=tc_user)


def action_crash(ips, phase, key, port, server_bin, dry_run):
    """Crash (kill -9) replica processes on specified nodes.

    Automatically caps the number of crashed nodes to f = (n-1)//3
    so the protocol can still make progress.
    """
    nodes = phase.get("nodes", [])
    n = len(ips)
    f = (n - 1) // 3
    # Filter to valid node indices and cap at f
    valid_nodes = [idx for idx in nodes if 1 <= idx <= n]
    if len(valid_nodes) > f:
        print(f"  [WARN] Scenario wants to crash {len(valid_nodes)} nodes, "
              f"but n={n} allows f={f}. Capping to first {f} nodes.")
        valid_nodes = valid_nodes[:f]
    print(f"  Crashing nodes: {valid_nodes} (n={n}, f={f})")
    cmd = f"killall -9 {server_bin}"
    run_on_nodes(ips, valid_nodes, cmd, key, port, dry_run)


def action_partition(ips, phase, key, port, dry_run):
    """Create a network partition between two groups using iptables."""
    group_a = phase.get("group_a", [])
    group_b = phase.get("group_b", [])
    print(f"  Creating partition: group_a={group_a} vs group_b={group_b}")

    group_a_ips = [ips[i - 1] for i in group_a if 1 <= i <= len(ips)]
    group_b_ips = [ips[i - 1] for i in group_b if 1 <= i <= len(ips)]

    threads = []
    # On each node in group_a, block traffic from/to group_b
    for ip_a in group_a_ips:
        for ip_b in group_b_ips:
            cmd = f"iptables -A INPUT -s {ip_b} -j DROP; iptables -A OUTPUT -d {ip_b} -j DROP"
            t = threading.Thread(target=ssh_cmd, args=(ip_a, cmd, key, port, dry_run))
            t.start()
            threads.append(t)

    # On each node in group_b, block traffic from/to group_a
    for ip_b in group_b_ips:
        for ip_a in group_a_ips:
            cmd = f"iptables -A INPUT -s {ip_a} -j DROP; iptables -A OUTPUT -d {ip_a} -j DROP"
            t = threading.Thread(target=ssh_cmd, args=(ip_b, cmd, key, port, dry_run))
            t.start()
            threads.append(t)

    for t in threads:
        t.join()


def action_heal(ips, phase, key, port, dry_run):
    """Heal network partition by flushing iptables rules."""
    print("  Healing partition: flushing iptables on all nodes")
    cmd = "iptables -F"
    run_on_all(ips, cmd, key, port, dry_run)


def action_set_delay_nodes(ips, phase, key, port, dry_run):
    """Set per-node delays for targeted attacks."""
    node_delays = phase.get("node_delays", {})
    threads = []
    for node_str, params in node_delays.items():
        node_idx = int(node_str)
        delay_ms = params.get("delay_ms", 40)
        jitter_ms = params.get("jitter_ms", 0)
        if 1 <= node_idx <= len(ips):
            ip = ips[node_idx - 1]
            tc_cmd = (f"tc qdisc change dev eth0 root netem delay {delay_ms}ms {jitter_ms}ms 2>/dev/null "
                      f"|| tc qdisc add dev eth0 root netem delay {delay_ms}ms {jitter_ms}ms")
            cmd = f"which tc >/dev/null 2>&1 && ({tc_cmd}) || echo '[WARN] tc not available, skipping delay'"
            print(f"  Node {node_idx} ({ip}): delay={delay_ms}ms jitter={jitter_ms}ms")
            t = threading.Thread(target=ssh_cmd, args=(ip, cmd, key, port, dry_run))
            t.start()
            threads.append(t)
    for t in threads:
        t.join()


def run_scenario(scenario, ips, key, port, server_bin, dry_run=False, tc_port=22, tc_user="Shaokang"):
    """Execute a scenario by processing phases in time order."""
    name = scenario.get("name", "unnamed")
    duration_s = scenario.get("duration_s", 120)
    phases = scenario.get("phases", [])

    print(f"\n{'='*60}")
    print(f"Scenario: {name}")
    print(f"Duration: {duration_s}s")
    print(f"Nodes: {len(ips)} IPs loaded")
    print(f"Phases: {len(phases)}")
    print(f"{'='*60}\n")

    # Sort phases by time
    phases = sorted(phases, key=lambda p: p.get("time_s", 0))

    start_time = time.time()

    for phase in phases:
        target_time = phase.get("time_s", 0)
        action = phase.get("action", "")

        # Wait until the right time
        elapsed = time.time() - start_time
        wait_time = target_time - elapsed
        if wait_time > 0 and not dry_run:
            print(f"[T+{target_time:>4}s] Waiting {wait_time:.1f}s...")
            time.sleep(wait_time)

        print(f"[T+{target_time:>4}s] Action: {action}")

        # tc commands run on host (tc_port) as tc_user with sudo
        # process commands (crash) run on container (port) as root
        if action == "set_delay":
            action_set_delay(ips, phase, key, tc_port, dry_run, tc_user=tc_user)
        elif action == "crash":
            action_crash(ips, phase, key, port, server_bin, dry_run)
        elif action == "partition":
            action_partition(ips, phase, key, tc_port, dry_run)
        elif action == "heal":
            action_heal(ips, phase, key, tc_port, dry_run)
        elif action == "set_delay_nodes":
            action_set_delay_nodes(ips, phase, key, tc_port, dry_run)
        else:
            print(f"  [WARN] Unknown action: {action}")

    # Wait for remaining duration
    elapsed = time.time() - start_time
    remaining = duration_s - elapsed
    if remaining > 0 and not dry_run:
        print(f"\n[T+{elapsed:.0f}s] Waiting {remaining:.1f}s for experiment to complete...")
        time.sleep(remaining)

    print(f"\n[T+{duration_s}s] Scenario '{name}' complete.")


def main():
    parser = argparse.ArgumentParser(description="Adversarial Network Experiment Scheduler")
    parser.add_argument("--scenario", required=True, help="Path to JSON scenario file")
    parser.add_argument("--ip-conf", required=True, help="Path to IP list .conf file")
    parser.add_argument("--key", required=True, help="SSH private key path")
    parser.add_argument("--server-bin", default="kv_server_performance", help="Server binary name")
    parser.add_argument("--ssh-port", type=int, default=2222, help="SSH port (default: 2222)")
    parser.add_argument("--tc-port", type=int, default=22, help="SSH port for tc commands on host (default: 22)")
    parser.add_argument("--tc-user", default="Shaokang", help="SSH user for tc commands on host (default: Shaokang)")
    parser.add_argument("--dry-run", action="store_true", help="Print commands without executing")
    args = parser.parse_args()

    # Load scenario
    with open(args.scenario) as f:
        scenario = json.load(f)

    # Parse IPs
    ips = parse_ip_conf(args.ip_conf)
    if not ips:
        print(f"ERROR: No IPs found in {args.ip_conf}", file=sys.stderr)
        sys.exit(1)

    client_num = parse_client_num(args.ip_conf)
    # Replica IPs are the first (total - client_num) entries
    replica_ips = ips[:len(ips) - client_num] if client_num < len(ips) else ips
    print(f"Loaded {len(ips)} IPs, {len(replica_ips)} replicas, {client_num} clients")

    run_scenario(scenario, replica_ips, args.key, args.ssh_port, args.server_bin,
                 args.dry_run, args.tc_port, args.tc_user)


if __name__ == "__main__":
    main()
