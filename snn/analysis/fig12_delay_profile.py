import csv
import json
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

ROOT = Path(__file__).resolve().parents[1]
HOLDOUT = ROOT.parent / "_archive" / "fig4_runs" / "fig4_independent_holdout_validation_20261017" / "runs" / "training"
RESULTS = ROOT / "results"
RESULTS.mkdir(exist_ok=True)
SEEDS = [730001, 730002, 730003, 730004, 730005, 730006]
UNITS = {0: {"source": 2, "left": 4, "right": 5}, 1: {"source": 3, "left": 6, "right": 7}}
COLORS = {730001: "#1f77b4", 730002: "#ff7f0e", 730003: "#2ca02c",
          730004: "#d62728", 730005: "#9467bd", 730006: "#8c564b"}


def read_weights(path):
    values = {}
    with open(path, newline="", encoding="utf-8") as f:
        for row in csv.DictReader(f):
            key = (int(row["source"]), int(row["target"]), int(row["delay_steps"]))
            values[key] = float(row["weight"])
    return values


def read_position(seed):
    with open(HOLDOUT / f"reward_{seed}" / "config.json", encoding="utf-8") as f:
        return float(json.load(f)["initial_position"])


def bounds(values):
    vals = list(values.values())
    return sum(v <= 1e-9 for v in vals), sum(v >= 2.0 - 1e-9 for v in vals)


rows = []
summary = []
for seed in SEEDS:
    run = HOLDOUT / f"reward_{seed}"
    w_init = read_weights(run / "weights_init.csv")
    w_final = read_weights(run / "weights_final.csv")
    position = read_position(seed)
    active_unit = 1 if position > 0 else 0
    at_zero, at_max = bounds(w_final)
    for unit, cfg in UNITS.items():
        diffs_init = []
        diffs_final = []
        positive = 0
        negative = 0
        for delay in range(10, 1001, 10):
            left_init = w_init[(cfg["source"], cfg["left"], delay)]
            right_init = w_init[(cfg["source"], cfg["right"], delay)]
            left_final = w_final[(cfg["source"], cfg["left"], delay)]
            right_final = w_final[(cfg["source"], cfg["right"], delay)]
            diff_init = right_init - left_init
            diff_final = right_final - left_final
            rows.append({
                "seed": seed,
                "unit": unit,
                "active_unit": 1 if unit == active_unit else 0,
                "source": cfg["source"],
                "target_left": cfg["left"],
                "target_right": cfg["right"],
                "delay_ms": delay // 10,
                "w_left_init": left_init,
                "w_right_init": right_init,
                "diff_init": diff_init,
                "w_left_final": left_final,
                "w_right_final": right_final,
                "diff_final": diff_final,
            })
            diffs_init.append(diff_init)
            diffs_final.append(diff_final)
            positive += diff_final > 0
            negative += diff_final < 0
        n = len(diffs_final)
        summary.append({
            "seed": seed,
            "initial_position": position,
            "unit": unit,
            "active_unit": 1 if unit == active_unit else 0,
            "mean_diff_init": sum(diffs_init) / n,
            "mean_diff_final": sum(diffs_final) / n,
            "fraction_positive_final": positive / n,
            "fraction_negative_final": negative / n,
            "weights_at_zero": at_zero,
            "weights_at_max": at_max,
        })

with open(RESULTS / "fig12_delay_profile_by_seed.csv", "w", newline="", encoding="utf-8") as f:
    writer = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
    writer.writeheader()
    writer.writerows(rows)

with open(RESULTS / "fig12_delay_profile_summary.csv", "w", newline="", encoding="utf-8") as f:
    writer = csv.DictWriter(f, fieldnames=list(summary[0].keys()))
    writer.writeheader()
    writer.writerows(summary)

for row in summary:
    if row["active_unit"] == 1:
        print(row)

fig, axes = plt.subplots(1, 2, figsize=(12, 4.6), sharey=True)
groups = [
    ("Right cue (paper Fig.12B sign)", [730003, 730004, 730006]),
    ("Left cue (mirror sign)", [730001, 730002, 730005]),
]
for ax, (title, seeds) in zip(axes, groups):
    delays = sorted({r["delay_ms"] for r in rows if r["seed"] == seeds[0] and r["active_unit"] == 1})
    for seed in seeds:
        curve = sorted([r for r in rows if r["seed"] == seed and r["active_unit"] == 1], key=lambda r: r["delay_ms"])
        ax.plot([r["delay_ms"] for r in curve], [r["diff_final"] for r in curve],
                color=COLORS[seed], linewidth=1.1, label=f"{seed} final")
    mean_init = []
    mean_final = []
    low = []
    high = []
    for delay in delays:
        init_values = []
        final_values = []
        for seed in seeds:
            row = next(r for r in rows if r["seed"] == seed and r["active_unit"] == 1 and r["delay_ms"] == delay)
            init_values.append(row["diff_init"])
            final_values.append(row["diff_final"])
        mean_init.append(sum(init_values) / len(init_values))
        mean_final.append(sum(final_values) / len(final_values))
        low.append(min(final_values))
        high.append(max(final_values))
    ax.plot(delays, mean_init, "k--", linewidth=1.1, label="mean init")
    ax.plot(delays, mean_final, "k-", linewidth=2.0, label="mean final")
    ax.fill_between(delays, low, high, color="gray", alpha=0.18, label="final min-max")
    ax.axhline(0, color="gray", linewidth=0.8)
    ax.set_title(title)
    ax.set_xlabel("synaptic delay (ms)")
    ax.grid(alpha=0.3)
    ax.legend(fontsize=7, ncol=2)
axes[0].set_ylabel("w(TurnRight) - w(TurnLeft)")
fig.tight_layout()
fig.savefig(RESULTS / "fig12_delay_profile.png", dpi=160)
plt.close(fig)

print("wrote", RESULTS / "fig12_delay_profile_by_seed.csv")
print("wrote", RESULTS / "fig12_delay_profile_summary.csv")
print("wrote", RESULTS / "fig12_delay_profile.png")
