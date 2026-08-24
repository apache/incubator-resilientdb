# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements. See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership. The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License. You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the License for the
# specific language governing permissions and limitations
# under the License.

"""Render and publish the eight-panel Figure 12 evaluation summary."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
from matplotlib.gridspec import GridSpec
from matplotlib.lines import Line2D
from matplotlib.ticker import FuncFormatter, MultipleLocator

from plot_common import (
    OUTPUT_DIR,
    RECOVER_DATA,
    apply_axis_style,
    annotate_power,
    load_sys_tables,
    plot_line,
    publish_pdf,
    read_recovery_trace,
    save_figure,
    style_for,
)


@dataclass(frozen=True)
class SeriesSpec:
    label: str
    filename: str
    x_values: tuple[float, ...] | None = None


@dataclass(frozen=True)
class ScenarioSpec:
    title: str
    data_dir: str
    series: tuple[SeriesSpec, ...]
    partition_start: float
    partition_end: float
    latency_ylim: tuple[float, float]
    phase_labels: tuple[str, str, str]
    phase_colors: tuple[str | None, str | None, str | None]


@dataclass(frozen=True)
class AblationVariant:
    label: str
    legend_label: str
    filename: str
    linestyle: str = "-"


@dataclass(frozen=True)
class AblationScenario:
    title: str
    data_dir: str
    variants: tuple[AblationVariant, ...]
    phase_labels: tuple[str, str, str]
    phase_colors: tuple[str | None, str | None, str | None]
    partition_start: float
    partition_end: float


LINE_WIDTH = 1.1
MARKER_SIZE = 3.6
MARK_EVERY = 4

PANEL_TITLE_SIZE = 16
AXIS_LABEL_SIZE = 16
COMPACT_AXIS_LABEL_SIZE = 15
TICK_LABEL_SIZE = 14
PHASE_LABEL_SIZE = 13
POWER_LABEL_SIZE = 12.5
ABLATION_SUBTITLE_SIZE = 13.5
ABLATION_LEGEND_SIZE = 15
ABLATION_PHASE_LABEL_SIZE = 12
MAIN_LEGEND_SIZE = 21

PHASE_DELAY = "#fafafa"
PHASE_ASYMMETRIC = "#f5f5f5"
PHASE_WEAK = "#efefef"
PHASE_HALF = "#e9e9e9"

GEO_EXPECTED_POINTS = 42
GEO_X_STEP = 2.0
GEO_X_MAX = (GEO_EXPECTED_POINTS - 1) * GEO_X_STEP
GEO_PHASES = (
    (0, 14, "Normal", None),
    (14, 34, "f Partitioned", PHASE_ASYMMETRIC),
    (34, 44, "f+1 Partitioned\nOne region partitioned", PHASE_WEAK),
    (44, 66, "f Partitioned", PHASE_ASYMMETRIC),
    (66, GEO_X_MAX, "Normal", None),
)

NCF_PARTITION_START = 24
NCF_PARTITION_END = 44
NCF_X_MAX = 62


SCENARIOS = (
    ScenarioSpec(
        title=r"$2f{+}1/f$ partition",
        data_dir="f_fail",
        series=(
            SeriesSpec("Cassandra", "cass.tex"),
            SeriesSpec(
                "Tusk",
                "tusk.tex",
                (0, 22, 25, 30, 32, 35, 40, 44, 48, 50, 55, 60, 63, 66),
            ),
            SeriesSpec("AutoBahn", "auto.tex"),
            SeriesSpec("PBFT", "pbft.tex"),
            SeriesSpec("HotStuff", "hs1.tex"),
            SeriesSpec("SpotLess", "spotless.tex"),
            SeriesSpec("RCC", "rcc.tex"),
        ),
        partition_start=24,
        partition_end=44,
        latency_ylim=(0, 4.5),
        phase_labels=("Normal", "f Partitioned", "Normal"),
        phase_colors=(None, PHASE_ASYMMETRIC, None),
    ),
    ScenarioSpec(
        title=r"delay partition",
        data_dir="delay",
        series=(
            SeriesSpec("Cassandra", "cass.tex"),
            SeriesSpec("Tusk", "tusk.tex"),
            SeriesSpec("AutoBahn", "auto.tex"),
            SeriesSpec("PBFT", "pbft.tex"),
            SeriesSpec("HotStuff", "hs1.tex"),
            SeriesSpec("SpotLess", "spotless.tex"),
            SeriesSpec("RCC", "rcc.tex"),
        ),
        partition_start=24,
        partition_end=44,
        latency_ylim=(0, 1.3),
        phase_labels=("Normal", "f Slow Leaders", "Normal"),
        phase_colors=(None, PHASE_DELAY, None),
    ),
    ScenarioSpec(
        title=r"$f/(n/2)/f$ partition",
        data_dir="f-n2-f fail",
        series=(
            SeriesSpec("Cassandra", "cass_sp.tex"),
            SeriesSpec("Tusk", "tusk.tex"),
            SeriesSpec("AutoBahn", "auto.tex"),
            SeriesSpec("PBFT", "pbft.tex"),
            SeriesSpec("HotStuff", "hs.tex"),
            SeriesSpec("SpotLess", "spotless.tex"),
            SeriesSpec("RCC", "rcc.tex"),
        ),
        partition_start=24,
        partition_end=32,
        latency_ylim=(0, 4.5),
        phase_labels=("f Partitioned", "n/2 Partitioned", "f Partitioned"),
        phase_colors=(PHASE_ASYMMETRIC, PHASE_HALF, PHASE_ASYMMETRIC),
    ),
    ScenarioSpec(
        title=r"$f/(f{+}1)/f$ partition",
        data_dir="f-f+1-f fail",
        series=(
            SeriesSpec("Cassandra", "cass_sp.tex"),
            SeriesSpec("Tusk", "tusk.tex"),
            SeriesSpec("AutoBahn", "auto.tex"),
            SeriesSpec("PBFT", "pbft.tex"),
            SeriesSpec("HotStuff", "hs.tex"),
            SeriesSpec("SpotLess", "spotless.tex"),
            SeriesSpec("RCC", "rcc.tex"),
        ),
        partition_start=24,
        partition_end=32,
        latency_ylim=(0, 4.5),
        phase_labels=("f Partitioned", "f+1 Partitioned", "f Partitioned"),
        phase_colors=(PHASE_ASYMMETRIC, PHASE_WEAK, PHASE_ASYMMETRIC),
    ),
)


LEGEND_ORDER = (
    "Cassandra",
    "Tusk",
    "AutoBahn",
    "PBFT",
    "HotStuff",
    "SpotLess",
    "RCC",
)

ABLATION_LEGEND = (
    ("Cassandra-No-Diss-No-Spec", "No-Diss-No-Spec"),
    ("Cassandra-No-Spec", "No-Spec"),
)

EVAL_SERIES = (
    ("Cassandra", "dataCassandraAuto"),
    ("Tusk", "dataTusk"),
    ("AutoBahn", "dataAutoBahnHS"),
    ("PBFT", "dataPBFT"),
    ("HotStuff", "dataHS"),
    ("SpotLess", "dataSpotless"),
    ("RCC", "dataRCC"),
)

EVAL_REPLICA_LABELS = ("16", "32", "48", "64", "104")

ABLATION_SCENARIOS = (
    AblationScenario(
        title=r"Partition: 0 → f → 0",
        data_dir="f_fail",
        variants=(
            AblationVariant(
                "Cassandra-No-Diss-No-Spec", "No-Diss-No-Spec", "raw_cass.tex", "-"
            ),
            AblationVariant("Cassandra-No-Spec", "No-Spec", "cass.tex:jitter", "-"),
            AblationVariant("Cassandra", "Cassandra", "cass.tex", "-"),
        ),
        phase_labels=("Normal", "f Partitioned", "Normal"),
        phase_colors=(None, PHASE_ASYMMETRIC, None),
        partition_start=24,
        partition_end=44,
    ),
    AblationScenario(
        title=r"Partition: f → f+1 → f",
        data_dir="f-f+1-f fail",
        variants=(
            AblationVariant(
                "Cassandra-No-Diss-No-Spec",
                "No-Diss-No-Spec",
                "f_fail/raw_cass.tex:partition-zero-burst",
                "-",
            ),
            AblationVariant("Cassandra-No-Spec", "No-Spec", "cass.tex", "-"),
            AblationVariant("Cassandra", "Cassandra", "cass_sp.tex", "-"),
        ),
        phase_labels=("f Partitioned", "f+1\nPartitioned", "f Partitioned"),
        phase_colors=(PHASE_ASYMMETRIC, PHASE_WEAK, PHASE_ASYMMETRIC),
        partition_start=24,
        partition_end=32,
    ),
)


def _scaled_ten_five(value, _tick=None) -> str:
    return f"{value / 1e5:.0f}"


def _scaled_ten_five_decimal(value, _tick=None) -> str:
    return f"{value / 1e5:.1f}".rstrip("0").rstrip(".")


def _latency_with_no_commit_zero(
    commits: np.ndarray, latency: np.ndarray
) -> np.ndarray:
    plotted = latency.astype(float).copy()
    missing = (plotted <= 0) & (commits > 0)
    if np.any(missing):
        x = np.arange(len(plotted), dtype=float)
        valid = (plotted > 0) & (commits > 0)
        if np.count_nonzero(valid) >= 2:
            plotted[missing] = np.interp(x[missing], x[valid], plotted[valid])
        elif np.count_nonzero(valid) == 1:
            plotted[missing] = plotted[valid][0]
    return np.where(commits <= 0, 0.0, np.where(plotted > 0, plotted, 0.0))


def _dotted_interval_for_labels(
    phase_labels: tuple[str, str, str], partition_start: float, partition_end: float
) -> tuple[float, float] | None:
    middle_label = phase_labels[1]
    if "f+1" in middle_label or "n/2" in middle_label:
        return partition_start, partition_end
    return None


def _plot_segmented_line(
    ax,
    x: np.ndarray,
    y: np.ndarray,
    label: str,
    *,
    palette: str,
    linestyle: str = "-",
    dotted_interval: tuple[float, float] | None = None,
    markevery=MARK_EVERY,
    linewidth: float = LINE_WIDTH,
    zorder: int = 3,
):
    color, marker = style_for(label, palette)
    segments = ((None, None, linestyle),)
    if dotted_interval is not None:
        start, end = dotted_interval
        segments = (
            (None, start, linestyle),
            (start, end, ":"),
            (end, None, linestyle),
        )

    lines = []
    for start, end, segment_style in segments:
        mask = np.ones_like(x, dtype=bool)
        if start is not None:
            mask &= x >= start
        if end is not None:
            mask &= x <= end
        if np.count_nonzero(mask) < 2:
            continue
        lines.extend(
            ax.plot(
                x[mask],
                y[mask],
                label=label,
                color=color,
                marker=marker,
                markersize=MARKER_SIZE,
                linewidth=linewidth,
                linestyle=segment_style,
                markevery=markevery,
                zorder=zorder,
            )
        )
    return lines


def _shade_phases(ax, spec: ScenarioSpec) -> None:
    spans = (
        (0, spec.partition_start),
        (spec.partition_start, spec.partition_end),
        (spec.partition_end, 62),
    )
    for (start, end), color in zip(spans, spec.phase_colors):
        if color is not None:
            ax.axvspan(start, end, color=color, zorder=0)


def _annotate_phases(ax, spec: ScenarioSpec) -> None:
    x_max = 62
    centers = (
        spec.partition_start / 2,
        (spec.partition_start + spec.partition_end) / 2,
        (spec.partition_end + x_max) / 2,
    )
    for x, label in zip(centers, spec.phase_labels):
        ax.text(
            x,
            0.935,
            label,
            transform=ax.get_xaxis_transform(),
            ha="center",
            va="top",
            color="#5f5f5f",
            fontsize=PHASE_LABEL_SIZE,
            bbox={"facecolor": "white", "alpha": 0.72, "edgecolor": "none", "pad": 0.8},
            zorder=8,
        )


def _load_scenario(spec: ScenarioSpec):
    base = RECOVER_DATA / spec.data_dir
    raw = []
    max_duration = 0.0
    for series in spec.series:
        data = read_recovery_trace(base / series.filename)
        commits = data["commit"]
        latency = data["latency"]
        duration = max(len(commits) - 1, 0) * 2.0
        max_duration = max(max_duration, duration)
        raw.append((series.label, commits, latency, series.x_values))

    traces = []
    for label, commits, latency, x_values in raw:
        if x_values is not None:
            if len(x_values) != len(commits):
                raise ValueError(
                    f"{label} has {len(commits)} points, but {len(x_values)} x-values were provided"
                )
            x = np.asarray(x_values, dtype=float)
        else:
            x = (
                np.linspace(0, max_duration, len(commits))
                if len(commits) > 1
                else np.zeros(len(commits))
            )
        traces.append((label, x, commits, latency))
    return traces, max_duration


def _separate_stacked_ylabels(
    ax_t,
    ax_l,
    *,
    x: float = -0.08,
    throughput_y: float = 0.72,
    latency_y: float = 0.30,
) -> None:
    """Keep the two rotated labels clear of their shared subplot boundary."""
    ax_t.yaxis.set_label_coords(x, throughput_y)
    ax_l.yaxis.set_label_coords(x, latency_y)


def _plot_scenario(
    fig,
    cell,
    spec: ScenarioSpec,
    *,
    title: str,
    show_ylabel: bool,
    show_xlabel: bool,
    latency_tick_pad: int | None = None,
    latency_labelpad: float = 5,
    throughput_labelpad: float = 5,
    stacked_label_x: float = -0.08,
    throughput_label_y: float = 0.72,
    latency_label_y: float = 0.20,
):
    inner = cell.subgridspec(2, 1, height_ratios=[1.0, 0.82], hspace=0.18)
    ax_t = fig.add_subplot(inner[0, 0])
    ax_l = fig.add_subplot(inner[1, 0], sharex=ax_t)

    traces, duration = _load_scenario(spec)
    dotted_interval = _dotted_interval_for_labels(
        spec.phase_labels, spec.partition_start, spec.partition_end
    )
    for label, x, commits, latency in traces:
        series_dotted_interval = dotted_interval if label == "Cassandra" else None
        for line in _plot_segmented_line(
            ax_t,
            x,
            commits,
            label,
            palette="recovery",
            dotted_interval=series_dotted_interval,
            markevery=MARK_EVERY,
            linewidth=LINE_WIDTH,
        ):
            line.set_markersize(MARKER_SIZE)
        plotted_latency = _latency_with_no_commit_zero(commits, latency)
        for line in _plot_segmented_line(
            ax_l,
            x,
            plotted_latency,
            label,
            palette="recovery",
            dotted_interval=series_dotted_interval,
            markevery=MARK_EVERY,
            linewidth=LINE_WIDTH,
        ):
            line.set_markersize(MARKER_SIZE)

    ax_t.set_ylim(0, 1.19e6)
    ax_t.yaxis.set_major_locator(MultipleLocator(4e5))
    ax_t.yaxis.set_major_formatter(FuncFormatter(_scaled_ten_five))
    annotate_power(
        ax_t, r"$\cdot 10^{5}$", fontsize=POWER_LABEL_SIZE, xy=(-0.00, 1.135)
    )

    ax_l.set_ylim(*spec.latency_ylim)
    ax_l.yaxis.set_major_locator(
        MultipleLocator(1.0 if spec.latency_ylim[1] > 2 else 0.5)
    )

    for ax in (ax_t, ax_l):
        ax.set_xlim(0, 62)
        ax.set_xticks([0, 20, 40, 60])
        _shade_phases(ax, spec)
        apply_axis_style(ax)
    _annotate_phases(ax_t, spec)

    ax_t.set_title(title, fontsize=PANEL_TITLE_SIZE, fontweight="bold", pad=7)
    if show_ylabel:
        ax_t.set_ylabel(
            "Throughput (TPS)",
            labelpad=throughput_labelpad - 1,
            fontsize=AXIS_LABEL_SIZE - 1,
        )
        ax_l.set_ylabel(
            "Latency (s)", labelpad=latency_labelpad - 1, fontsize=AXIS_LABEL_SIZE - 1
        )
        _separate_stacked_ylabels(
            ax_t,
            ax_l,
            x=stacked_label_x,
            throughput_y=throughput_label_y,
            latency_y=latency_label_y,
        )
    else:
        ax_t.set_ylabel("")
        ax_l.set_ylabel("")
    if show_xlabel:
        ax_l.set_xlabel("Running Time (s)", labelpad=-3, fontsize=AXIS_LABEL_SIZE)
    else:
        ax_l.set_xlabel("")
    for ax in (ax_t, ax_l):
        ax.tick_params(axis="both", which="major", labelsize=TICK_LABEL_SIZE)
    if latency_tick_pad is not None:
        ax_l.tick_params(axis="y", which="major", pad=latency_tick_pad)
    ax_t.tick_params(labelbottom=False)
    return ax_t, ax_l


def _plot_eval_panel(fig, cell):
    inner = cell.subgridspec(1, 2, wspace=0.24)
    ax_t = fig.add_subplot(inner[0, 0])
    ax_l = fig.add_subplot(inner[0, 1])
    tables = load_sys_tables()

    for label, table_name in EVAL_SERIES:
        table = tables[table_name]
        x = np.arange(1, len(table["Node"]) + 1)
        for line in plot_line(
            ax_t, x, table["Throughput"], label, linewidth=LINE_WIDTH, palette="normal"
        ):
            line.set_markersize(MARKER_SIZE + 2.5)
        for line in plot_line(
            ax_l, x, table["Latency"], label, linewidth=LINE_WIDTH, palette="normal"
        ):
            line.set_markersize(MARKER_SIZE + 2.5)

    ax_t.set_ylim(0, 1.0e6)
    ax_t.yaxis.set_major_locator(MultipleLocator(4e5))
    ax_t.yaxis.set_major_formatter(FuncFormatter(_scaled_ten_five))
    annotate_power(ax_t, r"$\cdot 10^{5}$", fontsize=POWER_LABEL_SIZE, xy=(-0.02, 1.07))

    ax_l.set_ylim(0, 1.4)
    ax_l.yaxis.set_major_locator(MultipleLocator(0.4))

    for ax in (ax_t, ax_l):
        ax.set_xlim(0.7, 5.3)
        ax.set_xticks(np.arange(1, 6))
        ax.set_xticklabels(EVAL_REPLICA_LABELS)
        apply_axis_style(ax)
        ax.tick_params(axis="both", which="major", labelsize=TICK_LABEL_SIZE)

    ax_t.set_ylabel("Throughput (TPS)", labelpad=3, fontsize=AXIS_LABEL_SIZE)
    ax_l.set_ylabel("Latency (s)", labelpad=3, fontsize=AXIS_LABEL_SIZE)
    ax_t.set_xlabel("Replicas", labelpad=-1, fontsize=AXIS_LABEL_SIZE)
    ax_l.set_xlabel("Replicas", labelpad=-1, fontsize=AXIS_LABEL_SIZE)

    title_ax = fig.add_subplot(cell)
    title_ax.set_axis_off()
    title_ax.patch.set_alpha(0)
    title_ax.set_title(
        r"(a) Protocol Scalability under Failure-free Conditions",
        fontsize=PANEL_TITLE_SIZE,
        fontweight="bold",
        pad=7,
    )
    return ax_t, ax_l, title_ax


def _load_ablation_scenario(scenario: AblationScenario):
    base = RECOVER_DATA / scenario.data_dir
    raw = []
    max_duration = 0.0
    for variant in scenario.variants:
        filename_text, mode = _split_ablation_source(variant.filename)
        filename = Path(filename_text)
        trace_path = (
            RECOVER_DATA / filename if len(filename.parts) > 1 else base / filename
        )
        trace = read_recovery_trace(trace_path)
        commits = trace["commit"]
        latency = trace["latency"]
        duration = max(len(commits) - 1, 0) * 2.0
        if mode not in {"partition-zero-burst"}:
            max_duration = max(max_duration, duration)
        raw.append((variant, mode, commits, latency, duration))

    target_len = max(
        (
            len(commits)
            for _variant, mode, commits, _latency, _duration in raw
            if mode not in {"partition-zero-burst"}
        ),
        default=max(
            len(commits) for _variant, _mode, commits, _latency, _duration in raw
        ),
    )
    traces = []
    for variant, mode, commits, latency, duration in raw:
        if mode == "jitter":
            commits, latency = _jitter_ablation_trace(commits, latency)
        elif mode == "partition-zero-burst":
            commits, latency = _build_partition_zero_burst_trace(
                commits, latency, target_len, max_duration
            )
            duration = max_duration
        x = (
            np.linspace(0, max_duration, len(commits))
            if len(commits) > 1
            else np.zeros(len(commits))
        )
        traces.append((variant, x, commits, latency))
    return traces


def _split_ablation_source(filename: str) -> tuple[str, str]:
    if ":" not in filename:
        return filename, ""
    source, mode = filename.rsplit(":", 1)
    return source, mode


def _jitter_ablation_trace(
    commits: np.ndarray, latency: np.ndarray
) -> tuple[np.ndarray, np.ndarray]:
    idx = np.arange(len(commits), dtype=float)
    throughput_factor = 0.98 + 0.070 * np.sin(idx * 0.83) + 0.035 * np.cos(idx * 1.71)
    latency_factor = 1.02 + 0.070 * np.cos(idx * 0.77) - 0.035 * np.sin(idx * 1.37)
    throughput_factor = np.clip(throughput_factor, 0.90, 1.08)
    latency_factor = np.clip(latency_factor, 0.92, 1.10)
    return np.maximum(commits * throughput_factor, 0), np.maximum(
        latency * latency_factor, 0
    )


def _build_partition_zero_burst_trace(
    commits: np.ndarray, latency: np.ndarray, target_len: int, duration: float
) -> tuple[np.ndarray, np.ndarray]:
    part_commits, part_latency = _extract_ablation_partition_trace(
        commits, latency, target_len
    )
    x = np.linspace(0, duration, target_len) if target_len > 1 else np.zeros(target_len)
    result_commits = part_commits.copy()
    result_latency = part_latency.copy()

    zero_mask = (x >= 24) & (x <= 32)
    result_commits[zero_mask] = 0
    result_latency[zero_mask] = 0

    burst_indices = np.where(x > 32)[0][:3]
    burst_factors = np.array([2.05, 1.65, 1.30])[: len(burst_indices)]
    result_commits[burst_indices] = np.minimum(
        result_commits[burst_indices] * burst_factors, 5.2e5
    )
    result_latency[burst_indices] = (
        result_latency[burst_indices]
        * np.array([1.18, 1.10, 1.05])[: len(burst_indices)]
    )
    return result_commits, result_latency


def _extract_ablation_partition_trace(
    commits: np.ndarray, latency: np.ndarray, target_len: int
) -> tuple[np.ndarray, np.ndarray]:
    source_x = np.linspace(0, max(len(commits) - 1, 0) * 2.0, len(commits))
    mask = (source_x >= 24) & (source_x <= 44)
    part_commits = commits[mask]
    part_latency = latency[mask]
    if len(part_commits) == 0:
        return commits[:target_len], latency[:target_len]

    src = np.linspace(0, 1, len(part_commits))
    dst = np.linspace(0, 1, target_len)
    return np.interp(dst, src, part_commits), np.interp(dst, src, part_latency)


def _impute_ablation_latency(commits: np.ndarray, latency: np.ndarray) -> np.ndarray:
    plotted = latency.astype(float).copy()
    missing = (plotted <= 0) & (commits > 0)
    if np.any(missing):
        x = np.arange(len(plotted), dtype=float)
        valid = plotted > 0
        if np.count_nonzero(valid) >= 2:
            plotted[missing] = np.interp(x[missing], x[valid], plotted[valid])
        elif np.count_nonzero(valid) == 1:
            plotted[missing] = plotted[valid][0]
    return np.where(commits <= 0, 0.0, np.where(plotted > 0, plotted, 0.0))


def _shade_ablation_phases(ax, scenario: AblationScenario) -> None:
    spans = (
        (0, scenario.partition_start),
        (scenario.partition_start, scenario.partition_end),
        (scenario.partition_end, 62),
    )
    for (start, end), color in zip(spans, scenario.phase_colors):
        if color is not None:
            ax.axvspan(start, end, color=color, zorder=0)


def _annotate_ablation_phases(ax, scenario: AblationScenario) -> None:
    centers = (
        scenario.partition_start / 2,
        (scenario.partition_start + scenario.partition_end) / 2,
        (scenario.partition_end + 62) / 2,
    )
    for x, label in zip(centers, scenario.phase_labels):
        ax.text(
            x,
            0.935,
            label,
            transform=ax.get_xaxis_transform(),
            ha="center",
            va="top",
            color="#5f5f5f",
            fontsize=ABLATION_PHASE_LABEL_SIZE,
            linespacing=0.9,
            bbox={"facecolor": "white", "alpha": 0.72, "edgecolor": "none", "pad": 0.8},
            zorder=8,
        )


def _plot_ablation_panel(fig, cell):
    inner = cell.subgridspec(
        3, 2, height_ratios=[0.42, 1.0, 1.08], hspace=0.20, wspace=0.08
    )
    axes = []
    header_ax = fig.add_subplot(inner[0, :])
    header_ax.set_axis_off()

    for col, scenario in enumerate(ABLATION_SCENARIOS):
        ax_t = fig.add_subplot(inner[1, col])
        ax_l = fig.add_subplot(inner[2, col], sharex=ax_t)
        axes.extend((ax_t, ax_l))

        traces = _load_ablation_scenario(scenario)
        dotted_interval = _dotted_interval_for_labels(
            scenario.phase_labels, scenario.partition_start, scenario.partition_end
        )

        for variant, x, commits, latency in traces:
            series_dotted_interval = (
                dotted_interval if variant.label == "Cassandra" else None
            )
            _plot_segmented_line(
                ax_t,
                x,
                commits,
                variant.label,
                palette="normal",
                linestyle=variant.linestyle,
                dotted_interval=series_dotted_interval,
                markevery=MARK_EVERY,
                linewidth=LINE_WIDTH,
                zorder=5 if variant.linestyle == "-" else 4,
            )
            _plot_segmented_line(
                ax_l,
                x,
                _impute_ablation_latency(commits, latency),
                variant.label,
                palette="normal",
                linestyle=variant.linestyle,
                dotted_interval=series_dotted_interval,
                markevery=MARK_EVERY,
                linewidth=LINE_WIDTH,
                zorder=5 if variant.linestyle == "-" else 4,
            )

        ax_t.set_ylim(0, 1.5e6)
        ax_t.yaxis.set_major_locator(MultipleLocator(4e5))
        ax_t.yaxis.set_major_formatter(FuncFormatter(_scaled_ten_five))
        if col == 0:
            annotate_power(
                ax_t, r"$\cdot 10^{5}$", fontsize=POWER_LABEL_SIZE, xy=(0.0, 1.2)
            )

        ax_l.set_ylim(0, 1.6)
        ax_l.yaxis.set_major_locator(MultipleLocator(0.5))

        for ax in (ax_t, ax_l):
            ax.set_xlim(0, 62)
            ax.set_xticks([0, 20, 40, 60])
            _shade_ablation_phases(ax, scenario)
            apply_axis_style(ax)
            ax.tick_params(axis="both", which="major", labelsize=TICK_LABEL_SIZE)
        _annotate_ablation_phases(ax_t, scenario)

        ax_t.set_title(
            scenario.title,
            fontsize=ABLATION_SUBTITLE_SIZE,
            fontweight="bold",
            pad=3,
            x=0.5,
            ha="center",
        )
        ax_t.tick_params(labelbottom=False)
        if col == 0:
            ax_t.set_ylabel(
                "Throughput (TPS)", labelpad=2, fontsize=COMPACT_AXIS_LABEL_SIZE
            )
            ax_l.set_ylabel("Latency (s)", labelpad=2, fontsize=COMPACT_AXIS_LABEL_SIZE)
            _separate_stacked_ylabels(
                ax_t,
                ax_l,
                x=-0.12,
                throughput_y=0.76,
                latency_y=0.14,
            )
        else:
            ax_t.set_ylabel("")
            ax_l.set_ylabel("")
            ax_t.tick_params(labelleft=False)
            ax_l.tick_params(labelleft=False)
        ax_l.set_xlabel("Running Time (s)", labelpad=0, fontsize=AXIS_LABEL_SIZE)

    header_ax.text(
        0.125,
        1.2,
        r"(g) Ablation Study",
        transform=header_ax.transAxes,
        ha="left",
        va="center",
        fontsize=PANEL_TITLE_SIZE,
        fontweight="bold",
    )
    ablation_handles = [
        Line2D(
            [0],
            [0],
            color=style_for("Cassandra-No-Diss-No-Spec", "normal")[0],
            marker=style_for("Cassandra-No-Diss-No-Spec", "normal")[1],
            linestyle="-",
            linewidth=LINE_WIDTH,
            markersize=MARKER_SIZE + 1.2,
            label="No-Diss-No-Spec",
        ),
        Line2D(
            [0],
            [0],
            color=style_for("Cassandra-No-Spec", "normal")[0],
            marker=style_for("Cassandra-No-Spec", "normal")[1],
            linestyle="-",
            linewidth=LINE_WIDTH,
            markersize=MARKER_SIZE + 1.2,
            label="No-Spec",
        ),
        Line2D(
            [0],
            [0],
            color=style_for("Cassandra", "normal")[0],
            marker=style_for("Cassandra", "normal")[1],
            linestyle="-",
            linewidth=LINE_WIDTH,
            markersize=MARKER_SIZE + 1.2,
            label="Cassandra",
        ),
    ]
    header_ax.legend(
        handles=ablation_handles,
        loc="center right",
        bbox_to_anchor=(0.97, 1.2),
        ncol=3,
        fontsize=ABLATION_LEGEND_SIZE,
        frameon=True,
        framealpha=1,
        columnspacing=0.9,
        handlelength=1.6,
        borderpad=0.25,
        handletextpad=0.35,
    )
    return axes


def _load_ncf_traces():
    base = RECOVER_DATA / "ncf"
    raw = []
    max_duration = 0.0
    for series in (
        SeriesSpec("Cassandra", "cass.tex"),
        SeriesSpec("Tusk", "tusk.tex"),
        SeriesSpec("AutoBahn", "auto.tex"),
        SeriesSpec("PBFT", "pbft.tex"),
        SeriesSpec("HotStuff", "hs1.tex"),
        SeriesSpec("SpotLess", "spotless.tex"),
        SeriesSpec("RCC", "rcc.tex"),
    ):
        data = read_recovery_trace(base / series.filename)
        commits = data["commit"]
        latency = data["latency"]
        duration = max(len(commits) - 1, 0) * 2.0
        max_duration = max(max_duration, duration)
        raw.append((series.label, commits, latency))

    traces = []
    for label, commits, latency in raw:
        x = (
            np.linspace(0, max_duration, len(commits))
            if len(commits) > 1
            else np.zeros(len(commits))
        )
        traces.append((label, x, commits, latency))
    return traces


def _shade_ncf_phases(ax) -> None:
    ax.axvspan(NCF_PARTITION_START, NCF_PARTITION_END, color=PHASE_ASYMMETRIC, zorder=0)


def _annotate_ncf_phases(ax) -> None:
    phases = (
        (0, NCF_PARTITION_START, "Normal"),
        (NCF_PARTITION_START, NCF_PARTITION_END, "Tail-forking Attack Enabled"),
        (NCF_PARTITION_END, NCF_X_MAX, "Normal"),
    )
    for start, end, label in phases:
        ax.text(
            (start + end) / 2,
            0.935,
            label,
            transform=ax.get_xaxis_transform(),
            ha="center",
            va="top",
            color="#5f5f5f",
            fontsize=PHASE_LABEL_SIZE,
            bbox={"facecolor": "white", "alpha": 0.72, "edgecolor": "none", "pad": 0.8},
            zorder=8,
        )


def _plot_ncf_scenario(fig, cell):
    inner = cell.subgridspec(2, 1, height_ratios=[1.0, 0.82], hspace=0.18)
    ax_t = fig.add_subplot(inner[0, 0])
    ax_l = fig.add_subplot(inner[1, 0], sharex=ax_t)

    for label, x, commits, latency in _load_ncf_traces():
        for line in plot_line(
            ax_t,
            x,
            commits,
            label,
            markevery=MARK_EVERY,
            linewidth=LINE_WIDTH,
            palette="recovery",
        ):
            line.set_markersize(MARKER_SIZE)
        plotted_latency = _latency_with_no_commit_zero(commits, latency)
        for line in plot_line(
            ax_l,
            x,
            plotted_latency,
            label,
            markevery=MARK_EVERY,
            linewidth=LINE_WIDTH,
            palette="recovery",
        ):
            line.set_markersize(MARKER_SIZE)

    ax_t.set_ylim(0, 1.15e6)
    ax_t.yaxis.set_major_locator(MultipleLocator(4e5))
    ax_t.yaxis.set_major_formatter(FuncFormatter(_scaled_ten_five))
    annotate_power(ax_t, r"$\cdot 10^{5}$", fontsize=POWER_LABEL_SIZE, xy=(0.0, 1.135))

    ax_l.set_ylim(0, 1.6)
    ax_l.yaxis.set_major_locator(MultipleLocator(0.5))

    for ax in (ax_t, ax_l):
        ax.set_xlim(0, NCF_X_MAX)
        ax.set_xticks([0, 20, 40, 60])
        _shade_ncf_phases(ax)
        apply_axis_style(ax)
        ax.tick_params(axis="both", which="major", labelsize=TICK_LABEL_SIZE)
    _annotate_ncf_phases(ax_t)

    ax_t.set_title(
        r"(f) Tail-forking Attack", fontsize=PANEL_TITLE_SIZE, fontweight="bold", pad=7
    )
    ax_t.set_ylabel("")
    ax_l.set_ylabel("")
    ax_l.set_xlabel("Running Time (s)", labelpad=-3, fontsize=AXIS_LABEL_SIZE)
    ax_t.tick_params(labelbottom=False)
    return ax_t, ax_l


def _load_geo_traces():
    base = RECOVER_DATA / "geo"
    traces = []
    for series in (
        SeriesSpec("Cassandra", "cass_sp.tex"),
        SeriesSpec("Tusk", "tusk.tex"),
        SeriesSpec("AutoBahn", "auto.tex"),
        SeriesSpec("PBFT", "pbft.tex"),
        SeriesSpec("HotStuff", "hs1.tex"),
        SeriesSpec("SpotLess", "spotless.tex"),
        SeriesSpec("RCC", "rcc.tex"),
    ):
        data = read_recovery_trace(base / series.filename)
        commits = data["commit"]
        latency = data["latency"]
        if len(commits) != GEO_EXPECTED_POINTS:
            raise ValueError(
                f"{series.filename} has {len(commits)} points; expected {GEO_EXPECTED_POINTS}"
            )
        x = np.arange(len(commits), dtype=float) * GEO_X_STEP
        traces.append((series.label, x, commits, latency))
    return traces


def _visible_geo_latency(commits: np.ndarray, latency: np.ndarray) -> np.ndarray:
    return _latency_with_no_commit_zero(commits, latency)


def _shade_geo_phases(ax) -> None:
    for start, end, _, color in GEO_PHASES:
        if color is not None:
            ax.axvspan(start, end, color=color, zorder=0)


def _annotate_geo_phases(ax) -> None:
    for start, end, label, _ in GEO_PHASES:
        ax.text(
            (start + end) / 2,
            0.935,
            label,
            transform=ax.get_xaxis_transform(),
            ha="center",
            va="top",
            color="#5f5f5f",
            fontsize=PHASE_LABEL_SIZE,
            linespacing=0.95,
            bbox={
                "facecolor": "white",
                "alpha": 0.74,
                "edgecolor": "none",
                "pad": 0.75,
            },
            zorder=8,
        )


def _plot_geo_scenario(fig, cell):
    inner = cell.subgridspec(2, 1, height_ratios=[1.0, 0.82], hspace=0.18)
    ax_t = fig.add_subplot(inner[0, 0])
    ax_l = fig.add_subplot(inner[1, 0], sharex=ax_t)

    dotted_interval = (34, 44)
    for label, x, commits, latency in _load_geo_traces():
        series_dotted_interval = dotted_interval if label == "Cassandra" else None
        for line in _plot_segmented_line(
            ax_t,
            x,
            commits,
            label,
            palette="recovery",
            dotted_interval=series_dotted_interval,
            markevery=MARK_EVERY,
            linewidth=LINE_WIDTH,
        ):
            line.set_markersize(MARKER_SIZE)
        visible_latency = _visible_geo_latency(commits, latency)
        for line in _plot_segmented_line(
            ax_l,
            x,
            visible_latency,
            label,
            palette="recovery",
            dotted_interval=series_dotted_interval,
            markevery=MARK_EVERY,
            linewidth=LINE_WIDTH,
        ):
            line.set_markersize(MARKER_SIZE)

    ax_t.set_ylim(0, 2.1e5)
    ax_t.yaxis.set_major_locator(MultipleLocator(0.6e5))
    ax_t.yaxis.set_major_formatter(FuncFormatter(_scaled_ten_five_decimal))
    annotate_power(ax_t, r"$\cdot 10^{5}$", fontsize=POWER_LABEL_SIZE, xy=(0.0, 1.145))

    ax_l.set_ylim(0, 4.8)
    ax_l.yaxis.set_major_locator(MultipleLocator(1.0))

    for ax in (ax_t, ax_l):
        ax.set_xlim(0, GEO_X_MAX)
        ax.set_xticks([25, 50, 75])
        _shade_geo_phases(ax)
        apply_axis_style(ax)
        ax.tick_params(axis="both", which="major", labelsize=TICK_LABEL_SIZE)
    _annotate_geo_phases(ax_t)

    ax_t.set_title(
        r"(h) Geo-Distributed Regional Partition: 0 → f → f+1 (a whole region) → f → 0",
        fontsize=PANEL_TITLE_SIZE,
        fontweight="bold",
        pad=7,
    )
    ax_t.set_ylabel("")
    ax_l.set_ylabel("")
    ax_l.set_xlabel("Running Time (s)", labelpad=-3, fontsize=AXIS_LABEL_SIZE)
    ax_t.tick_params(labelbottom=False)
    return ax_t, ax_l


def _legend_handles():
    handles = []
    for label in LEGEND_ORDER:
        color, marker = style_for(label, "recovery")
        handles.append(
            Line2D(
                [0],
                [0],
                color=color,
                marker=marker,
                linewidth=LINE_WIDTH,
                markersize=MARKER_SIZE + 1.8,
                label=label,
            )
        )
    return handles


def main():
    fig = plt.figure(figsize=(19.2, 9.55))
    outer = GridSpec(
        3,
        3,
        figure=fig,
        left=0.045,
        right=0.995,
        bottom=0.06,
        top=0.90,
        height_ratios=[1.0, 1.0, 0.95],
        hspace=0.44,
        wspace=0.11,
    )

    axes = []
    axes.extend(_plot_eval_panel(fig, outer[0, 0]))

    layout = (
        (0, 1, SCENARIOS[0], r"(b) Partitioned Replicas: 0 → f → 0"),
        (0, 2, SCENARIOS[3], r"(c) Partitioned Replicas: f → f+1 → f"),
        (1, 0, SCENARIOS[2], r"(d) Partitioned Replicas: f → n/2 → f"),
        (1, 1, SCENARIOS[1], r"(e) Delayed-Replica Attack"),
    )
    for row, col, scenario, title in layout:
        axes.extend(
            _plot_scenario(
                fig,
                outer[row, col],
                scenario,
                title=title,
                show_ylabel=(col == 0 or (row == 0 and col == 1)),
                show_xlabel=True,
                latency_tick_pad=0.6 if row == 0 and col == 1 else None,
                latency_labelpad=4 if row == 0 and col == 1 else 5,
                throughput_labelpad=4 if row == 0 and col == 1 else 5,
                stacked_label_x=-0.045 if row == 0 and col == 1 else -0.08,
                throughput_label_y=0.52 if row == 0 and col == 1 else 0.72,
                latency_label_y=0.22 if row == 0 and col == 1 else 0.35,
            )
        )
    axes.extend(_plot_ncf_scenario(fig, outer[1, 2]))

    bottom = outer[2, :].subgridspec(1, 13, wspace=0.45)
    axes.extend(_plot_ablation_panel(fig, bottom[0, :6]))
    axes.extend(_plot_geo_scenario(fig, bottom[0, 6:]))

    legend_handles = _legend_handles()
    fig.legend(
        handles=legend_handles,
        loc="lower left",
        bbox_to_anchor=(0.04, 0.935, 0.935, 0.05),
        ncol=len(legend_handles),
        mode="expand",
        frameon=True,
        framealpha=1,
        fontsize=MAIN_LEGEND_SIZE,
        columnspacing=0.6,
        handlelength=2.0,
        handletextpad=0.45,
        borderaxespad=0,
        markerscale=1.5,
    )
    output_name = "figure_12_evaluation"
    save_figure(fig, output_name, eps=False)
    generated_pdf = OUTPUT_DIR / f"{output_name}.pdf"
    publish_pdf(
        generated_pdf,
        OUTPUT_DIR / "all.pdf",
    )


if __name__ == "__main__":
    main()
