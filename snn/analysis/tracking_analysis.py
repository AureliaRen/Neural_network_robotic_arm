import csv
import os
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

ROOT = Path(__file__).resolve().parents[1]
RUNS = ROOT / "runs"
RESULTS = ROOT / "results"
RESULTS.mkdir(exist_ok=True)

SEEDS = [910001, 910002, 910003, 910004, 910005, 910006]
MODES = ["reward", "punishment"]


def read_one(path):
    with open(path, newline="", encoding="utf-8") as f:
        rows = list(csv.DictReader(f))
    if len(rows) != 1:
        raise RuntimeError(f"expected one row in {path}, got {len(rows)}")
    return rows[0]


def weight_bounds(path):
    with open(path, newline="", encoding="utf-8") as f:
        rows = list(csv.DictReader(f))
    values = [float(r["weight"]) for r in rows]
    n = len(values)
    lower = sum(v <= 1e-9 for v in values)
    upper = sum(v >= 2.0 - 1e-9 for v in values)
    return n, lower, upper


records = []
for mode in MODES:
    for seed in SEEDS:
        train_dir = RUNS / f"tracking_train_{mode}_{seed}"
        init_dir = RUNS / f"tracking_eval_init_{seed}"
        final_dir = RUNS / f"tracking_eval_final_{mode}_{seed}"
        train = read_one(train_dir / "tracking_summary.csv")
        init = read_one(init_dir / "tracking_summary.csv")
        final = read_one(final_dir / "tracking_summary.csv")
        n_w, lower, upper = weight_bounds(train_dir / "weights_final.csv")
        init_mae = float(init["mean_abs_distance"])
        final_mae = float(final["mean_abs_distance"])
        records.append({
            "mode": mode,
            "seed": seed,
            "train_arrivals": int(train["arrivals"]),
            "train_learning_blocks": int(train["learning_blocks"]),
            "train_no_decisions": int(train["no_decisions"]),
            "train_mae": float(train["mean_abs_distance"]),
            "max_weight_change": float(train["max_weight_change"]),
            "init_eval_mae": init_mae,
            "init_eval_arrivals": int(init["arrivals"]),
            "init_eval_no_decisions": int(init["no_decisions"]),
            "final_eval_mae": final_mae,
            "final_eval_arrivals": int(final["arrivals"]),
            "final_eval_no_decisions": int(final["no_decisions"]),
            "delta_mae": final_mae - init_mae,
            "improved": int(final_mae < init_mae),
            "weights": n_w,
            "weights_at_zero": lower,
            "weights_at_max": upper,
        })

per_seed = RESULTS / "tracking_per_seed.csv"
with open(per_seed, "w", newline="", encoding="utf-8") as f:
    writer = csv.DictWriter(f, fieldnames=list(records[0].keys()))
    writer.writeheader()
    writer.writerows(records)

aggregate = []
for mode in MODES:
    subset = [r for r in records if r["mode"] == mode]
    n = len(subset)
    aggregate.append({
        "mode": mode,
        "n": n,
        "mean_init_mae": sum(r["init_eval_mae"] for r in subset) / n,
        "mean_final_mae": sum(r["final_eval_mae"] for r in subset) / n,
        "mean_delta_mae": sum(r["delta_mae"] for r in subset) / n,
        "n_improved": sum(r["improved"] for r in subset),
        "mean_init_arrivals": sum(r["init_eval_arrivals"] for r in subset) / n,
        "mean_final_arrivals": sum(r["final_eval_arrivals"] for r in subset) / n,
        "mean_weights_at_bound": sum(r["weights_at_zero"] + r["weights_at_max"] for r in subset) / n,
    })

with open(RESULTS / "tracking_aggregate.csv", "w", newline="", encoding="utf-8") as f:
    writer = csv.DictWriter(f, fieldnames=list(aggregate[0].keys()))
    writer.writeheader()
    writer.writerows(aggregate)

fig, axes = plt.subplots(1, 2, figsize=(11, 4.2), sharey=True)
for ax, mode in zip(axes, MODES):
    subset = [r for r in records if r["mode"] == mode]
    x = list(range(len(subset)))
    width = 0.38
    ax.bar([i - width / 2 for i in x], [r["init_eval_mae"] for r in subset], width, label="initial weights")
    ax.bar([i + width / 2 for i in x], [r["final_eval_mae"] for r in subset], width, label=f"{mode}-trained weights")
    ax.set_xticks(x)
    ax.set_xticklabels([str(r["seed"]) for r in subset], rotation=30)
    ax.set_title(f"Moving-cue tracking: {mode}")
    ax.set_ylabel("mean absolute distance (virtual units)")
    ax.grid(axis="y", alpha=0.3)
    ax.legend(fontsize=8)
fig.tight_layout()
fig.savefig(RESULTS / "tracking_mae.png", dpi=160)
plt.close(fig)

seed = 910001
init_dir = RUNS / f"tracking_eval_init_{seed}"
final_dir = RUNS / f"tracking_eval_final_reward_{seed}"


def read_trajectory(directory):
    with open(directory / "tracking_trajectories.csv", newline="", encoding="utf-8") as f:
        rows = list(csv.DictReader(f))
    return {int(r["cycle"]): r for r in rows}


with open(init_dir / "tracking_cue.csv", newline="", encoding="utf-8") as f:
    cue_rows = list(csv.DictReader(f))
targets = {int(r["cycle"]): float(r["target"]) for r in cue_rows}
init_rows = read_trajectory(init_dir)
final_rows = read_trajectory(final_dir)
cycles = sorted(targets)
init_pos = [float(init_rows[c]["position_after"]) for c in cycles]
final_pos = [float(final_rows[c]["position_after"]) for c in cycles]
target_seq = [targets[c] for c in cycles]

window_ids = sorted(set(c // 20 for c in cycles))
init_window_mae = []
final_window_mae = []
for w in window_ids:
    selected = [c for c in cycles if c // 20 == w]
    init_window_mae.append(sum(abs(float(init_rows[c]["position_after"]) - targets[c]) for c in selected) / len(selected))
    final_window_mae.append(sum(abs(float(final_rows[c]["position_after"]) - targets[c]) for c in selected) / len(selected))

fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(11, 7), gridspec_kw={"height_ratios": [2, 1]})
ax1.step(cycles, target_seq, where="post", color="black", linewidth=1.6, label="cue target")
ax1.plot(cycles, init_pos, color="#888888", linewidth=1.0, label="initial weights")
ax1.plot(cycles, final_pos, color="#d62728", linewidth=1.0, label="reward-trained weights")
ax1.set_ylabel("position")
ax1.set_title(f"Fig.13-style moving cue, seed {seed}, triangle protocol")
ax1.grid(alpha=0.3)
ax1.legend(loc="upper right", fontsize=8)

ax2.plot(window_ids, init_window_mae, marker="o", color="#888888", label="initial weights")
ax2.plot(window_ids, final_window_mae, marker="o", color="#d62728", label="reward-trained weights")
ax2.set_xlabel("cue window (20 cycles each)")
ax2.set_ylabel("window mean |error|")
ax2.grid(alpha=0.3)
ax2.legend(fontsize=8)
fig.tight_layout()
fig.savefig(RESULTS / "tracking_trajectory_910001.png", dpi=160)
plt.close(fig)

print("wrote", per_seed)
print("wrote", RESULTS / "tracking_aggregate.csv")
print("wrote", RESULTS / "tracking_mae.png")
print("wrote", RESULTS / "tracking_trajectory_910001.png")
for row in aggregate:
    print(row)
