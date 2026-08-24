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

"""Shared paths, styles, data loaders, and export helpers for evaluation plots."""

from __future__ import annotations

import re
import shutil
from pathlib import Path

import matplotlib

matplotlib.use("Agg")

import matplotlib.pyplot as plt
import numpy as np
from matplotlib.ticker import FuncFormatter


FIGURE_ROOT = Path(__file__).resolve().parent
FIGURE_DIR = FIGURE_ROOT / "data"
OUTPUT_DIR = FIGURE_ROOT / "output"
SYS_DATA = FIGURE_DIR / "scalability_data.tex"
RECOVER_DATA = FIGURE_DIR / "recover_data"

GRID_COLOR = "#eaeaea"
MARKER_SIZE = 7
LINE_WIDTH = 2.2
LABEL_PAD = 10

NORMAL_COLORS = {
    "Cassandra": "#ff7f7f",
    "Cassandra-Raw": "#5aa6d1",
    "Cassandra-SP": "#ff7f7f",
    "Cassandra-No-Diss-No-Spec": "#5aa6d1",
    "Cassandra-No-Spec": "#4daf4a",
    "AutoBahn": "#a8d99b",
    "AutoBahn+HotStuff": "#6bbd6b",
    "HotStuff": "#af8981",
    "HotStuff-1": "#af8981",
    "PBFT": "#8a8a8a",
    "Tusk": "#f6b26b",
    "RCC": "#b495d1",
    "SpotLess": "#e8daa7",
    "Zzy": "#c8c8c8",
    "MultiPaxos": "#c8c8c8",
    "Cassandra-CFT": "#6bbd6b",
}

NORMAL_MARKERS = {
    "Cassandra": "^",
    "Cassandra-Raw": "s",
    "Cassandra-SP": "^",
    "Cassandra-No-Diss-No-Spec": "s",
    "Cassandra-No-Spec": "v",
    "AutoBahn": "o",
    "AutoBahn+HotStuff": "s",
    "HotStuff": "*",
    "HotStuff-1": "*",
    "PBFT": "p",
    "Tusk": "D",
    "RCC": "P",
    "SpotLess": "X",
    "Zzy": ">",
    "MultiPaxos": "o",
    "Cassandra-CFT": "^",
}

RECOVERY_COLORS = {
    **NORMAL_COLORS,
    "No-Quorum": "#8a8a8a",
    "Weak-Quorum": "#eba0d4",
    "Strong-Quorum": "#feac64",
    "PoA": "#6bbd6b",
    "PoR": "#af8981",
}

RECOVERY_MARKERS = {
    **NORMAL_MARKERS,
    "No-Quorum": "o",
    "Weak-Quorum": "s",
    "Strong-Quorum": "D",
    "PoA": "s",
    "PoR": "D",
    "MultiPaxos": "o",
    "Cassandra-CFT": "^",
}

COLORS = RECOVERY_COLORS
MARKERS = RECOVERY_MARKERS


def configure_matplotlib() -> None:
    plt.rcParams.update(
        {
            "pdf.fonttype": 42,
            "ps.fonttype": 42,
            "font.family": "serif",
            "font.serif": ["Times New Roman", "Times", "DejaVu Serif"],
            "mathtext.fontset": "stix",
            "mathtext.default": "rm",
            "font.size": 19,
            "axes.titlesize": 19,
            "axes.labelsize": 19,
            "xtick.labelsize": 17,
            "ytick.labelsize": 17,
            "legend.fontsize": 17,
        }
    )


def apply_axis_style(ax, *, minor: bool = True) -> None:
    ax.set_axisbelow(True)
    ax.grid(True, which="major", color=GRID_COLOR, linewidth=0.8 if minor else 1.0)
    if minor:
        ax.grid(True, which="minor", color=GRID_COLOR, linewidth=0.5, alpha=0.6)
    for spine in ax.spines.values():
        spine.set_linewidth(0.8)


def format_million(value, _tick=None) -> str:
    return f"{value / 1e6:.1f}".rstrip("0").rstrip(".")


def format_thousand(value, _tick=None) -> str:
    return f"{value / 1e3:.0f}"


def format_ten_thousand(value, _tick=None) -> str:
    return f"{value / 1e4:.1f}".rstrip("0").rstrip(".")


def annotate_power(ax, text=r"$\cdot 10^{6}$", xy=(0.0, 1.08), fontsize=15) -> None:
    ax.annotate(
        text, xy=xy, xycoords="axes fraction", fontsize=fontsize, ha="left", va="top"
    )


def save_figure(fig, name: str, out_dir: Path = OUTPUT_DIR, eps: bool = True) -> None:
    out_dir.mkdir(parents=True, exist_ok=True)
    pdf_path = out_dir / f"{name}.pdf"
    png_path = out_dir / f"{name}.png"
    fig.savefig(pdf_path, dpi=500, bbox_inches="tight")
    fig.savefig(png_path, dpi=500, bbox_inches="tight")
    normalize_png(png_path)
    if eps:
        fig.savefig(out_dir / f"{name}.eps", dpi=500, bbox_inches="tight")
    plt.close(fig)


def publish_pdf(source: Path, *destinations: Path) -> None:
    """Copy a generated PDF to the stable locations consumed by the paper."""
    for destination in destinations:
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, destination)


def normalize_png(path: Path) -> None:
    try:
        from PIL import Image
    except ImportError:
        return
    with Image.open(path) as image:
        image.convert("RGB").save(path, "PNG", optimize=False, compress_level=6)


def safe_log(values, floor: float = 1.0) -> np.ndarray:
    return np.maximum(np.asarray(values, dtype=float), floor)


def load_sys_tables(path: Path = SYS_DATA) -> dict[str, dict[str, np.ndarray]]:
    text = path.read_text()
    text = "\n".join(
        line for line in text.splitlines() if not line.lstrip().startswith("%")
    )
    tables: dict[str, dict[str, np.ndarray]] = {}
    pattern = re.compile(
        r"\\pgfplotstableread\s*\{(.*?)\}\s*\\(data[A-Za-z0-9]+)", re.S
    )
    for match in pattern.finditer(text):
        body, name = match.groups()
        lines = [line.strip() for line in body.splitlines() if line.strip()]
        if not lines:
            continue
        header = re.split(r"\s+", lines[0])
        cols = {col: [] for col in header}
        for line in lines[1:]:
            parts = re.split(r"\s+", line)
            if len(parts) < len(header):
                continue
            for col, value in zip(header, parts):
                try:
                    cols[col].append(float(value))
                except ValueError:
                    cols[col].append(np.nan)
        tables[name] = {
            col: np.asarray(vals, dtype=float) for col, vals in cols.items()
        }
    return tables


def read_recovery_trace(path: Path) -> dict[str, np.ndarray]:
    commits, latencies = [], []
    for line in path.read_text().splitlines():
        match = re.search(r"commit:([0-9.]+)\s+(?:client\s+)?latency:([0-9.]+)", line)
        if not match:
            continue
        commits.append(float(match.group(1)))
        latencies.append(float(match.group(2)))
    return {
        "commit": np.asarray(commits, dtype=float),
        "latency": np.asarray(latencies, dtype=float),
    }


def style_for(label: str, palette: str = "recovery") -> tuple[str | None, str]:
    if palette == "normal":
        return NORMAL_COLORS.get(label), NORMAL_MARKERS.get(label, "o")
    return RECOVERY_COLORS.get(label), RECOVERY_MARKERS.get(label, "o")


def plot_line(
    ax,
    x,
    y,
    label: str,
    *,
    markevery=None,
    zorder=3,
    linewidth=LINE_WIDTH,
    palette: str = "recovery",
):
    color, marker = style_for(label, palette)
    return ax.plot(
        x,
        y,
        label=label,
        color=color,
        marker=marker,
        markersize=MARKER_SIZE,
        linewidth=linewidth,
        markevery=markevery,
        zorder=zorder,
    )


def add_partition_span(
    ax, start, end, label="Network Partition", y=None, y_frac=0.9, fontsize=17
) -> None:
    ax.axvspan(start, end, color="#f0f0f0", zorder=0)
    ymin, ymax = ax.get_ylim()
    if y is not None:
        text_y = y
    elif ax.get_yscale() == "log":
        y = 10 ** (np.log10(ymin) + (np.log10(ymax) - np.log10(ymin)) * y_frac)
        text_y = y
    else:
        text_y = ymin + (ymax - ymin) * y_frac
    ax.text(
        (start + end) / 2,
        text_y,
        label,
        ha="center",
        va="bottom",
        color="gray",
        style="italic",
        fontsize=fontsize,
    )


def annotate_recovery(
    ax, x, y, *, text="Recovery", color="#7cd1d1", xytext=None, show_marker=True
) -> None:
    if show_marker:
        ax.scatter(
            [x], [y], s=140, marker="*", color=color, zorder=8, label="_nolegend_"
        )
    if xytext is None:
        xytext = (x + 2, y * 6)
    ax.annotate(
        text,
        xy=(x, y),
        xytext=xytext,
        fontsize=17,
        fontweight="bold",
        ha="right",
        va="center",
        arrowprops=dict(
            arrowstyle="-|>",
            color="black",
            lw=1.2,
            shrinkB=5,
            connectionstyle="arc3,rad=0",
        ),
        zorder=9,
    )


def finalize_legend(
    fig, axes, ncol: int, x: float = 0.5, y: float = 1.02, fontsize: int | None = None
) -> None:
    if not isinstance(axes, (list, tuple, np.ndarray)):
        axes = [axes]
    handles, labels = [], []
    seen = set()
    for ax in axes:
        for handle, label in zip(*ax.get_legend_handles_labels()):
            if label not in seen and not label.startswith("_"):
                handles.append(handle)
                labels.append(label)
                seen.add(label)
    fig.legend(
        handles,
        labels,
        loc="upper center",
        bbox_to_anchor=(x, y),
        ncol=ncol,
        frameon=True,
        framealpha=1,
        fontsize=fontsize,
    )


configure_matplotlib()
