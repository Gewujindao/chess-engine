"""Draw figures/summary.png: search-node savings and perft correctness.

All numbers are constants copied from results/03_engine_tests.txt
(the output of the last full test run); nothing is read from disk.
Run from the repository root: python figures/make_figures.py
"""
from pathlib import Path

import matplotlib.pyplot as plt
from matplotlib.ticker import FixedLocator, FuncFormatter, NullLocator

# Shared style
BLUE, GRAY, ACCENT = "#1f4e79", "#9aa5b1", "#d98c1f"
INK, MUTED, GRID = "#1a1a1a", "#5f6b78", "#e9ecef"
FS_TITLE, FS_LABEL, FS_SMALL = 12, 9, 8
plt.rcParams.update({
    "font.family": "sans-serif",
    "font.sans-serif": ["Arial", "Helvetica", "DejaVu Sans"],
    "mathtext.fontset": "custom",
    "mathtext.rm": "Arial",
    "font.size": FS_SMALL,
    "axes.linewidth": 0.6,
    "axes.edgecolor": INK,
    "xtick.direction": "out",
    "ytick.direction": "out",
    "xtick.major.width": 0.6,
    "ytick.major.width": 0.6,
    "xtick.major.size": 3,
    "ytick.major.size": 3,
    "xtick.color": INK,
    "ytick.color": INK,
})

# Alpha-beta vs unpruned minimax, same evaluate(), 53 fixed-depth searches
N_SEARCHES = 53           # results/03_engine_tests.txt lines 56-108 (53 rows, all PASS)
MINIMAX_NODES = 19759032  # results/03_engine_tests.txt line 109
ALPHABETA_NODES = 165622  # results/03_engine_tests.txt line 109

# Perft from the starting position: (depth, engine nodes); expected == engine, all PASS
PERFT_STARTPOS = [
    (1, 20),          # results/03_engine_tests.txt line 13
    (2, 400),         # line 14
    (3, 8902),        # line 15
    (4, 197281),      # line 16
    (5, 4865609),     # line 17
    (6, 119060324),   # line 18
]


def pow10(v, _):
    return f"$\\mathregular{{10^{{{len(str(int(round(v)))) - 1}}}}}$"


def style(ax, ticks):
    for s in ("top", "right"):
        ax.spines[s].set_visible(False)
    ax.set_yscale("log")
    ax.yaxis.set_major_locator(FixedLocator(ticks))
    ax.yaxis.set_minor_locator(NullLocator())
    ax.yaxis.set_major_formatter(FuncFormatter(pow10))
    ax.yaxis.grid(True, color=GRID, lw=0.6, zorder=0)
    ax.set_axisbelow(True)
    ax.tick_params(axis="both", labelsize=FS_SMALL)


def label(fig, x, y, letter, title):
    fig.text(x, y, letter, fontsize=FS_LABEL, fontweight="bold", color=INK,
             ha="left", va="center")
    fig.text(x + 0.022, y, title, fontsize=FS_LABEL, color=INK, ha="left", va="center")


def main():
    fig = plt.figure(figsize=(8, 4.5), dpi=200, facecolor="white")
    fig.text(0.02, 0.945, "Alpha-beta matched minimax on every search with far fewer "
             "nodes; perft is exact",
             fontsize=FS_TITLE, fontweight="bold", color=INK, ha="left", va="center")
    fig.text(0.02, 0.885, "Same evaluation function at fixed depth; perft compared with "
             "published node counts; last full test run",
             fontsize=FS_LABEL, color=MUTED, ha="left", va="center")

    # a: total nodes, log scale
    ax1 = fig.add_axes([0.10, 0.10, 0.30, 0.64])
    vals = [MINIMAX_NODES, ALPHABETA_NODES]
    ax1.bar([0, 1], vals, width=0.58, color=[GRAY, BLUE], zorder=2)
    style(ax1, [1e4, 1e5, 1e6, 1e7, 1e8])
    ax1.set_ylim(1e4, 1e8)
    for x, v in zip([0, 1], vals):
        ax1.text(x, v * 1.3, f"{v:,}", ha="center", va="bottom",
                 fontsize=FS_LABEL, color=INK, fontweight="bold")
    ax1.set_xticks([0, 1], ["Minimax", "Alpha-beta"], fontsize=FS_LABEL)
    ax1.set_xlim(-0.55, 1.55)
    ax1.tick_params(axis="x", length=0)
    ax1.set_ylabel("Total nodes searched", fontsize=FS_LABEL, color=INK)
    ax1.text(1, 3.5e6, f"Same score in\nall {N_SEARCHES} searches", ha="center",
             va="center", fontsize=FS_SMALL, color=BLUE, linespacing=1.3)

    # b: perft node counts at the starting position
    ax2 = fig.add_axes([0.575, 0.10, 0.40, 0.64])
    depths = [d for d, _ in PERFT_STARTPOS]
    nodes = [n for _, n in PERFT_STARTPOS]
    ax2.plot(depths, nodes, color=BLUE, lw=1.6, marker="o", ms=5.5, zorder=3,
             markeredgecolor="white", markeredgewidth=1.0)
    style(ax2, [1e1, 1e3, 1e5, 1e7, 1e9])
    ax2.set_ylim(5, 2e9)
    for d, n in PERFT_STARTPOS:
        ax2.annotate(f"{n:,}", (d, n), xytext=(-6, 4), textcoords="offset points",
                     ha="right", va="bottom", fontsize=FS_SMALL, color=INK)
    ax2.set_xticks(depths)
    ax2.set_xlim(0.5, 6.4)
    ax2.set_xlabel("Depth (plies)", fontsize=FS_LABEL, color=INK)
    ax2.set_ylabel("Leaf nodes", fontsize=FS_LABEL, color=INK)
    ax2.text(6.35, 12, "All 6 depths equal\nthe published counts", ha="right",
             va="bottom", fontsize=FS_SMALL, color=BLUE, linespacing=1.3)

    label(fig, 0.02, 0.795, "a", f"Total nodes over {N_SEARCHES} searches")
    label(fig, 0.495, 0.795, "b", "Perft from the starting position")

    out = Path(__file__).resolve().parent / "summary.png"
    fig.savefig(out, dpi=200, facecolor="white")


if __name__ == "__main__":
    main()
