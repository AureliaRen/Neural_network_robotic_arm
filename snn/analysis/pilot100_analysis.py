import csv
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

ROOT = Path(__file__).resolve().parents[1]
RUNS = ROOT / "runs"
RESULTS = ROOT / "results"
HOLDOUT = ROOT.parent / "_archive" / "fig4_runs" / "fig4_independent_holdout_validation_20261017" / "runs" / "training"
SEEDS = [730001, 730002, 730003, 730004, 730005, 730006]
UNITS = {0: {"source": 2, "left": 4, "right": 5}, 1: {"source": 3, "left": 6, "right": 7}}


def read_rows(path):
    with open(path, newline="", encoding="utf-8") as f:
        return list(csv.DictReader(f))


def read_one(path):
    rows = read_rows(path)
    if len(rows) != 1:
        raise RuntimeError(f"expected one row in {path}, got {len(rows)}")
    return rows[0]


def read_weights(path):
    out = {}
    for row in read_rows(path):
        key = (int(row["source"]), int(row["target"]), int(row["delay_steps"]))
        out[key] = float(row["weight"])
    return out


def read_position(directory):
    with open(directory / "config.json", encoding="utf-8") as f:
        text = f.read()
    for line in text.splitlines():
        if '"initial_position"' in line:
            return float(line.split(":")[1].strip().rstrip(","))
    raise RuntimeError(f"no initial_position in {directory}")


def bounds(values):
    return sum(v <= 1e-9 for v in values), sum(v >= 2.0 - 1e-9 for v in values)


def fig12_diff(weights, active_unit):
    unit = UNITS[active_unit]
    diffs = []
    for delay in range(10, 1001, 10):
        left = weights[(unit["source"], unit["left"], delay)]
        right = weights[(unit["source"], unit["right"], delay)]
        diffs.append(right - left)
    return sum(diffs) / len(diffs), sum(d > 0 for d in diffs) / len(diffs)


rows = []
for seed in SEEDS:
    pilot_dir = RUNS / f"pilot100_reward_{seed}"
    static_dir = RUNS / f"pilot100_eval6_static_{seed}"
    moving_dir = RUNS / f"pilot100_eval6_triangle_{seed}"
    holdout_dir = HOLDOUT / f"reward_{seed}"

    pilot_train = read_one(pilot_dir / "summary.csv")
    pilot_static = read_rows(static_dir / "closed_evaluation.csv")
    pilot_moving = read_one(moving_dir / "tracking_summary.csv")
    pilot_init = read_weights(pilot_dir / "weights_init.csv")
    pilot_final = read_weights(pilot_dir / "weights_final.csv")
    holdout_final = read_weights(holdout_dir / "weights_final.csv")

    position = read_position(pilot_dir)
    active_unit = 1 if position > 0 else 0
    pilot_diff, pilot_frac = fig12_diff(pilot_final, active_unit)
    holdout_diff, holdout_frac = fig12_diff(holdout_final, active_unit)
    pilot_zero, pilot_max = bounds(pilot_final.values())
    holdout_zero, holdout_max = bounds(holdout_final.values())

    static_holdout = [r for r in read_rows(RESULTS / "data" / "static_holdout_by_seed.csv") if int(r["seed"]) == seed][0]
    moving_holdout = [r for r in read_rows(RESULTS / "data" / "moving_holdout_by_seed.csv") if int(r["seed"]) == seed and r["protocol"] == "triangle"][0]

    rows.append({
        "seed": seed,
        "initial_position": position,
        "active_unit": active_unit,
        "pilot_arrivals": int(pilot_train["arrivals"]),
        "pilot_learning_blocks": int(pilot_train["learning_blocks"]),
        "pilot_max_weight_change": float(pilot_train["max_weight_change"]),
        "pilot_static_approach": sum(float(r["approach_success"]) for r in pilot_static) / len(pilot_static),
        "holdout_static_approach": float(static_holdout["reward_final"]),
        "pilot_triangle_mae": float(pilot_moving["mean_abs_distance"]),
        "pilot_triangle_arrivals": int(pilot_moving["arrivals"]),
        "holdout_triangle_mae": float(moving_holdout["mae_final"]),
        "pilot_weights_at_zero": pilot_zero,
        "pilot_weights_at_max": pilot_max,
        "holdout_weights_at_zero": holdout_zero,
        "holdout_weights_at_max": holdout_max,
        "pilot_fig12_mean_diff": pilot_diff,
        "pilot_fig12_fraction_positive": pilot_frac,
        "holdout_fig12_mean_diff": holdout_diff,
        "holdout_fig12_fraction_positive": holdout_frac,
    })

with open(RESULTS / "pilot100_by_seed.csv", "w", newline="", encoding="utf-8") as f:
    writer = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
    writer.writeheader()
    writer.writerows(rows)

n = len(rows)
summary = {
    "seeds": n,
    "pilot_static_approach_mean": sum(r["pilot_static_approach"] for r in rows) / n,
    "holdout_static_approach_mean": sum(r["holdout_static_approach"] for r in rows) / n,
    "pilot_triangle_mae_mean": sum(r["pilot_triangle_mae"] for r in rows) / n,
    "holdout_triangle_mae_mean": sum(r["holdout_triangle_mae"] for r in rows) / n,
    "pilot_weights_at_bound_mean": sum(r["pilot_weights_at_zero"] + r["pilot_weights_at_max"] for r in rows) / n,
    "holdout_weights_at_bound_mean": sum(r["holdout_weights_at_zero"] + r["holdout_weights_at_max"] for r in rows) / n,
    "pilot_fig12_abs_mean": sum(abs(r["pilot_fig12_mean_diff"]) for r in rows) / n,
    "holdout_fig12_abs_mean": sum(abs(r["holdout_fig12_mean_diff"]) for r in rows) / n,
    "pilot_static_improved": sum(r["pilot_static_approach"] > 0.5 for r in rows),
    "pilot_triangle_beat_control": sum(r["pilot_triangle_mae"] < 6.894 for r in rows),
}
with open(RESULTS / "pilot100_summary.csv", "w", newline="", encoding="utf-8") as f:
    writer = csv.DictWriter(f, fieldnames=list(summary.keys()))
    writer.writeheader()
    writer.writerow(summary)

fig, axes = plt.subplots(1, 2, figsize=(12, 4.5))
x = list(range(n))
width = 0.38
ax = axes[0]
ax.bar([i - width / 2 for i in x], [r["pilot_static_approach"] for r in rows], width, label="100 cycles", color="#1f77b4")
ax.bar([i + width / 2 for i in x], [r["holdout_static_approach"] for r in rows], width, label="300 cycles (holdout)", color="#7f7f7f")
ax.set_xticks(x); ax.set_xticklabels([str(r["seed"]) for r in rows], rotation=30)
ax.set_ylabel("static approach success"); ax.set_ylim(0, 1.05)
ax.set_title("Static reward: 100 vs 300 cycles")
ax.grid(axis="y", alpha=0.3); ax.legend(fontsize=8)

ax = axes[1]
ax.bar([i - width / 2 for i in x], [r["pilot_triangle_mae"] for r in rows], width, label="100 cycles", color="#ff7f0e")
ax.bar([i + width / 2 for i in x], [r["holdout_triangle_mae"] for r in rows], width, label="300 cycles (holdout)", color="#7f7f7f")
ax.set_xticks(x); ax.set_xticklabels([str(r["seed"]) for r in rows], rotation=30)
ax.set_ylabel("moving triangle MAE")
ax.set_title("Moving triangle: 100 vs 300 cycles")
ax.grid(axis="y", alpha=0.3); ax.legend(fontsize=8)
fig.tight_layout()
fig.savefig(RESULTS / "pilot100_comparison.png", dpi=160)
plt.close(fig)

print("wrote", RESULTS / "pilot100_by_seed.csv")
print("wrote", RESULTS / "pilot100_summary.csv")
print("wrote", RESULTS / "pilot100_comparison.png")
for key, value in summary.items():
    print(key, value)
for row in rows:
    print(row["seed"], "static", round(row["pilot_static_approach"], 3), "mae", round(row["pilot_triangle_mae"], 3),
          "bounds", row["pilot_weights_at_zero"], row["pilot_weights_at_max"],
          "fig12", round(row["pilot_fig12_mean_diff"], 3), "arr", row["pilot_arrivals"])
