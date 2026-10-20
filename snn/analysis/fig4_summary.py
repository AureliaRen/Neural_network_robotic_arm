import csv
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

ROOT = Path(__file__).resolve().parents[1]
DATA = ROOT / "results" / "data"
RESULTS = ROOT / "results"


def read_rows(path):
    with open(path, newline="", encoding="utf-8") as f:
        return list(csv.DictReader(f))


static = read_rows(DATA / "static_holdout_by_seed.csv")
tracking = [r for r in read_rows(RESULTS / "tracking_per_seed.csv") if r["mode"] == "reward"]
moving = read_rows(DATA / "moving_holdout_by_seed.csv")

fig, axes = plt.subplots(2, 2, figsize=(12, 8))
width = 0.38

ax = axes[0, 0]
x = list(range(len(static)))
ax.bar([i - width / 2 for i in x], [float(r["reward_init"]) for r in static], width, label="initial", color="#999999")
ax.bar([i + width / 2 for i in x], [float(r["reward_final"]) for r in static], width, label="reward-trained", color="#2ca02c")
ax.set_xticks(x); ax.set_xticklabels([r["seed"] for r in static], rotation=30)
ax.set_ylim(0, 1.05); ax.set_ylabel("approach success")
ax.set_title("Static reward (holdout): 6/6 improved")
ax.grid(axis="y", alpha=0.3); ax.legend(fontsize=8)

ax = axes[0, 1]
ax.bar([i - width / 2 for i in x], [float(r["punishment_init"]) for r in static], width, label="initial", color="#999999")
ax.bar([i + width / 2 for i in x], [float(r["punishment_final"]) for r in static], width, label="punishment-trained", color="#d62728")
ax.set_xticks(x); ax.set_xticklabels([r["seed"] for r in static], rotation=30)
ax.set_ylim(0, 1.05); ax.set_ylabel("strict avoidance")
ax.set_title("Static punishment (holdout): 6/6 improved")
ax.grid(axis="y", alpha=0.3); ax.legend(fontsize=8)

x2 = list(range(len(tracking)))
ax = axes[1, 0]
ax.bar([i - width / 2 for i in x2], [float(r["init_eval_mae"]) for r in tracking], width, label="initial", color="#999999")
ax.bar([i + width / 2 for i in x2], [float(r["final_eval_mae"]) for r in tracking], width, label="reward-trained", color="#1f77b4")
ax.set_xticks(x2); ax.set_xticklabels([r["seed"] for r in tracking], rotation=30)
ax.set_ylabel("moving-cue MAE")
ax.set_title("Online moving cue, reward: MAE 6/6 improved")
ax.grid(axis="y", alpha=0.3); ax.legend(fontsize=8)

ax = axes[1, 1]
ax.bar([i - width / 2 for i in x2], [float(r["init_eval_arrivals"]) for r in tracking], width, label="initial", color="#999999")
ax.bar([i + width / 2 for i in x2], [float(r["final_eval_arrivals"]) for r in tracking], width, label="reward-trained", color="#1f77b4")
ax.set_xticks(x2); ax.set_xticklabels([r["seed"] for r in tracking], rotation=30)
ax.set_ylabel("target arrivals / 500 cycles")
ax.set_title("Online moving cue, reward: arrivals 6/6 improved")
ax.grid(axis="y", alpha=0.3); ax.legend(fontsize=8)

fig.tight_layout()
fig.savefig(RESULTS / "fig4_combined_summary.png", dpi=160)
plt.close(fig)

fig, axes = plt.subplots(1, 2, figsize=(11, 4.2), sharey=True)
for ax, protocol in zip(axes, ["triangle", "wrap"]):
    subset = [r for r in moving if r["protocol"] == protocol]
    x = list(range(len(subset)))
    ax.bar([i - width / 2 for i in x], [float(r["mae_init"]) for r in subset], width, label="initial", color="#999999")
    ax.bar([i + width / 2 for i in x], [float(r["mae_final"]) for r in subset], width, label="static-trained", color="#9467bd")
    ax.set_xticks(x); ax.set_xticklabels([r["seed"] for r in subset], rotation=30)
    ax.set_title(f"Static-trained weights, {protocol} cue")
    ax.set_ylabel("moving-cue MAE")
    ax.grid(axis="y", alpha=0.3)
    ax.legend(fontsize=8)
fig.tight_layout()
fig.savefig(RESULTS / "fig4_moving_static.png", dpi=160)
plt.close(fig)

print("wrote", RESULTS / "fig4_combined_summary.png")
print("wrote", RESULTS / "fig4_moving_static.png")
