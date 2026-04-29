#!/usr/bin/env python3
"""
Visualization script for adversarial experiment results.

Generates publication-quality figures for BFT protocol comparison papers.

Usage:
  python3 plot_results.py --results-dir adversarial_results/ --output figures/

Requires: matplotlib, numpy (install with: pip3 install matplotlib numpy)
"""

import argparse
import csv
import json
import os
import sys

try:
    import matplotlib
    matplotlib.use('Agg')  # Non-interactive backend
    import matplotlib.pyplot as plt
    import numpy as np
except ImportError:
    print("ERROR: matplotlib and numpy required. Install with: pip3 install matplotlib numpy")
    sys.exit(1)

# Paper-friendly style
plt.rcParams.update({
    'font.size': 12,
    'font.family': 'serif',
    'axes.labelsize': 14,
    'axes.titlesize': 14,
    'xtick.labelsize': 11,
    'ytick.labelsize': 11,
    'legend.fontsize': 11,
    'figure.figsize': (8, 5),
    'figure.dpi': 300,
    'savefig.bbox': 'tight',
    'savefig.pad_inches': 0.1,
})

PROTOCOL_COLORS = {
    'fides': '#2196F3',
    'damysus': '#FF5722',
    'achilles': '#4CAF50',
    'hotstuff': '#9C27B0',
    'pbft': '#FF9800',
}

PROTOCOL_MARKERS = {
    'fides': 'o',
    'damysus': 's',
    'achilles': '^',
    'hotstuff': 'D',
    'pbft': 'v',
}


def load_timeseries_csv(filepath):
    """Load a timeseries CSV file."""
    data = []
    with open(filepath) as f:
        reader = csv.DictReader(f)
        for row in reader:
            entry = {}
            for k, v in row.items():
                try:
                    entry[k] = float(v) if v else 0
                except ValueError:
                    entry[k] = v
            data.append(entry)
    return data


def find_result_dirs(base_dir, pattern=""):
    """Find result directories matching a pattern."""
    dirs = []
    if not os.path.isdir(base_dir):
        return dirs
    for name in sorted(os.listdir(base_dir)):
        path = os.path.join(base_dir, name)
        if os.path.isdir(path) and (not pattern or pattern in name):
            dirs.append(path)
    return dirs


def plot_throughput_timeseries(csv_files, labels, output_path, title="Throughput Over Time"):
    """Plot throughput time-series for multiple experiments/protocols."""
    fig, ax = plt.subplots()

    for csv_file, label in zip(csv_files, labels):
        data = load_timeseries_csv(csv_file)
        times = [d.get('time_s', 0) for d in data]
        tps = [d.get('tps', 0) for d in data]

        protocol = label.split('_')[0].lower() if '_' in label else label.lower()
        color = PROTOCOL_COLORS.get(protocol, '#333333')
        marker = PROTOCOL_MARKERS.get(protocol, 'o')

        ax.plot(times, tps, label=label, color=color, marker=marker,
                markersize=4, linewidth=1.5)

    ax.set_xlabel('Time (seconds)')
    ax.set_ylabel('Throughput (txn/s)')
    ax.set_title(title)
    ax.legend()
    ax.grid(True, alpha=0.3)
    ax.set_ylim(bottom=0)

    plt.savefig(output_path)
    plt.close()
    print(f"  Saved: {output_path}")


def plot_throughput_vs_crashes(crash_data, output_path):
    """Plot throughput degradation vs number of crash faults.

    crash_data: dict of {protocol: [(num_crashes, avg_tps), ...]}
    """
    fig, ax = plt.subplots()

    for protocol, points in crash_data.items():
        points.sort(key=lambda x: x[0])
        crashes = [p[0] for p in points]
        tps = [p[1] for p in points]

        color = PROTOCOL_COLORS.get(protocol, '#333333')
        marker = PROTOCOL_MARKERS.get(protocol, 'o')
        ax.plot(crashes, tps, label=protocol.capitalize(), color=color,
                marker=marker, markersize=6, linewidth=2)

    ax.set_xlabel('Number of Crash Faults')
    ax.set_ylabel('Average Throughput (txn/s)')
    ax.set_title('Throughput vs. Crash Faults')
    ax.legend()
    ax.grid(True, alpha=0.3)
    ax.set_ylim(bottom=0)

    plt.savefig(output_path)
    plt.close()
    print(f"  Saved: {output_path}")


def plot_throughput_vs_latency(latency_data, output_path):
    """Plot throughput under different network delays.

    latency_data: dict of {protocol: [(delay_ms, avg_tps), ...]}
    """
    fig, ax = plt.subplots()

    for protocol, points in latency_data.items():
        points.sort(key=lambda x: x[0])
        delays = [p[0] for p in points]
        tps = [p[1] for p in points]

        color = PROTOCOL_COLORS.get(protocol, '#333333')
        marker = PROTOCOL_MARKERS.get(protocol, 'o')
        ax.plot(delays, tps, label=protocol.capitalize(), color=color,
                marker=marker, markersize=6, linewidth=2)

    ax.set_xlabel('Network Delay (ms)')
    ax.set_ylabel('Average Throughput (txn/s)')
    ax.set_title('Throughput vs. Network Delay')
    ax.legend()
    ax.grid(True, alpha=0.3)
    ax.set_xscale('log')
    ax.set_ylim(bottom=0)

    plt.savefig(output_path)
    plt.close()
    print(f"  Saved: {output_path}")


def plot_ablation(ablation_data, output_path):
    """Plot TEE component ablation study.

    ablation_data: dict of {config_name: avg_tps}
    """
    fig, ax = plt.subplots(figsize=(8, 5))

    configs = list(ablation_data.keys())
    tps_values = [ablation_data[c] for c in configs]

    colors = ['#2196F3', '#64B5F6', '#90CAF9', '#BBDEFB'][:len(configs)]
    bars = ax.bar(configs, tps_values, color=colors, edgecolor='black', linewidth=0.5)

    # Add value labels on bars
    for bar, val in zip(bars, tps_values):
        ax.text(bar.get_x() + bar.get_width() / 2, bar.get_height() + max(tps_values) * 0.02,
                f'{val:.0f}', ha='center', va='bottom', fontsize=10)

    ax.set_ylabel('Average Throughput (txn/s)')
    ax.set_title('TEE Component Ablation Study')
    ax.set_ylim(0, max(tps_values) * 1.15)
    ax.grid(True, alpha=0.3, axis='y')

    plt.savefig(output_path)
    plt.close()
    print(f"  Saved: {output_path}")


def plot_phase_comparison(phase_data, output_path):
    """Plot bar chart comparing phases across protocols.

    phase_data: dict of {protocol: {phase_name: avg_tps}}
    """
    fig, ax = plt.subplots(figsize=(10, 5))

    protocols = list(phase_data.keys())
    if not protocols:
        return
    phases = list(phase_data[protocols[0]].keys())
    n_protocols = len(protocols)
    n_phases = len(phases)

    x = np.arange(n_phases)
    width = 0.8 / n_protocols

    for i, protocol in enumerate(protocols):
        values = [phase_data[protocol].get(phase, 0) for phase in phases]
        color = PROTOCOL_COLORS.get(protocol, '#333333')
        offset = (i - n_protocols / 2 + 0.5) * width
        ax.bar(x + offset, values, width, label=protocol.capitalize(),
               color=color, edgecolor='black', linewidth=0.5)

    ax.set_xlabel('Experiment Phase')
    ax.set_ylabel('Average Throughput (txn/s)')
    ax.set_title('Per-Phase Throughput Comparison')
    ax.set_xticks(x)
    ax.set_xticklabels(phases, rotation=15)
    ax.legend()
    ax.grid(True, alpha=0.3, axis='y')
    ax.set_ylim(bottom=0)

    plt.savefig(output_path)
    plt.close()
    print(f"  Saved: {output_path}")


def auto_plot(results_dir, output_dir):
    """Auto-detect result directories and generate appropriate plots."""
    os.makedirs(output_dir, exist_ok=True)

    # Find all timeseries CSVs
    csv_files = []
    labels = []
    for root, dirs, files in os.walk(results_dir):
        for f in files:
            if f == 'timeseries.csv':
                csv_path = os.path.join(root, f)
                label = os.path.basename(root)
                csv_files.append(csv_path)
                labels.append(label)

    if csv_files:
        print(f"\nFound {len(csv_files)} timeseries CSV files")

        # Group by scenario type
        scenario_groups = {}
        for csv_file, label in zip(csv_files, labels):
            # Extract scenario name (e.g., "gst_ramp" from "gst_ramp_20260327_120000")
            parts = label.rsplit('_', 2)
            scenario_name = parts[0] if len(parts) >= 3 else label
            if scenario_name not in scenario_groups:
                scenario_groups[scenario_name] = ([], [])
            scenario_groups[scenario_name][0].append(csv_file)
            scenario_groups[scenario_name][1].append(label)

        for scenario_name, (files, lbls) in scenario_groups.items():
            output_path = os.path.join(output_dir, f"timeseries_{scenario_name}.png")
            plot_throughput_timeseries(files, lbls, output_path,
                                      title=f"Throughput: {scenario_name}")
    else:
        print("No timeseries CSV files found. Run experiments first.")
        print(f"Looked in: {results_dir}")

    # Generate example figure templates with placeholder data
    print("\nGenerating example figure templates...")

    # Example: Throughput vs crashes
    example_crash_data = {
        'fides': [(0, 50000), (1, 45000), (2, 38000), (3, 30000), (4, 22000), (5, 15000)],
        'damysus': [(0, 48000), (1, 42000), (2, 5000), (3, 3000), (4, 1000), (5, 500)],
    }
    plot_throughput_vs_crashes(example_crash_data,
                              os.path.join(output_dir, "example_crash_faults.png"))

    # Example: Throughput vs latency
    example_latency_data = {
        'fides': [(40, 50000), (100, 35000), (200, 20000), (500, 8000), (1000, 3000), (2000, 1200)],
        'damysus': [(40, 48000), (100, 30000), (200, 15000), (500, 2000), (1000, 500), (2000, 0)],
    }
    plot_throughput_vs_latency(example_latency_data,
                               os.path.join(output_dir, "example_throughput_vs_latency.png"))

    # Example: Ablation
    example_ablation = {
        'Fides(full)': 50000,
        'Fides(-MC)': 42000,
        'Fides(-RAC)': 46000,
        'Fides(-RNG)': 44000,
    }
    plot_ablation(example_ablation, os.path.join(output_dir, "example_ablation.png"))

    print(f"\nAll figures saved to: {output_dir}")


def main():
    parser = argparse.ArgumentParser(description="Plot adversarial experiment results")
    parser.add_argument("--results-dir", default="adversarial_results",
                        help="Directory containing experiment results")
    parser.add_argument("--output", default="figures",
                        help="Output directory for figures")
    args = parser.parse_args()

    auto_plot(args.results_dir, args.output)


if __name__ == "__main__":
    main()
