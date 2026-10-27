import csv
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

ROOT = Path(__file__).resolve().parents[1]
RUNS = ROOT / "runs"
RESULTS = ROOT / "results"
SEEDS = [910001, 910002, 910003, 910004, 910005, 910006]


def read_one(path):
    with open(path, newline="", encoding="utf-8") as f:
        rows = list(csv.DictReader(f))
    if len(rows) != 1:
        raise RuntimeError(path)
    return rows[0]


def weight_bounds(path):
    with open(path, newline="", encoding="utf-8") as f:
        rows = list(csv.DictReader(f))
    values = [float(r["weight"]) for r in rows]
    return len(values), sum(v <= 1e-9 for v in values), sum(v >= 2.0 - 1e-9 for v in values)


wrap = []
for seed in SEEDS:
    train = read_one(RUNS / f"tracking_wrap_train_reward_{seed}" / "tracking_summary.csv")
    init = read_one(RUNS / f"tracking_wrap_eval_init_{seed}" / "tracking_summary.csv")
    final = read_one(RUNS / f"tracking_wrap_eval_final_reward_{seed}" / "tracking_summary.csv")
    n_w, lower, upper = weight_bounds(RUNS / f"tracking_wrap_train_reward_{seed}" / "weights_final.csv")
    init_mae = float(init["mean_abs_distance"])
    final_mae = float(final["mean_abs_distance"])
    wrap.append({
        "protocol": "wrap",
        "mode": "reward",
        "seed": seed,
        "train_arrivals": int(train["arrivals"]),
        "train_mae": float(train["mean_abs_distance"]),
        "max_weight_change": float(train["max_weight_change"]),
        "init_eval_mae": init_mae,
        "init_eval_arrivals": int(init["arrivals"]),
        "final_eval_mae": final_mae,
        "final_eval_arrivals": int(final["arrivals"]),
        "delta_mae": final_mae - init_mae,
        "improved": int(final_mae < init_mae),
        "weights": n_w,
        "weights_at_zero": lower,
        "weights_at_max": upper,
    })

with open(RESULTS / "tracking_wrap_per_seed.csv", "w", newline="", encoding="utf-8") as f:
    writer = csv.DictWriter(f, fieldnames=list(wrap[0].keys()))
    writer.writeheader()
    writer.writerows(wrap)

n = len(wrap)
wrap_agg = {
    "protocol": "wrap",
    "n": n,
    "mean_init_mae": sum(r["init_eval_mae"] for r in wrap) / n,
    "mean_final_mae": sum(r["final_eval_mae"] for r in wrap) / n,
    "mean_delta_mae": sum(r["delta_mae"] for r in wrap) / n,
    "n_improved": sum(r["improved"] for r in wrap),
    "mean_init_arrivals": sum(r["init_eval_arrivals"] for r in wrap) / n,
    "mean_final_arrivals": sum(r["final_eval_arrivals"] for r in wrap) / n,
    "mean_weights_at_bound": sum(r["weights_at_zero"] + r["weights_at_max"] for r in wrap) / n,
}

with open(RESULTS / "tracking_wrap_aggregate.csv", "w", newline="", encoding="utf-8") as f:
    writer = csv.DictWriter(f, fieldnames=list(wrap_agg.keys()))
    writer.writeheader()
    writer.writerow(wrap_agg)

with open(RESULTS / "tracking_per_seed.csv", newline="", encoding="utf-8") as f:
    triangle = [r for r in csv.DictReader(f) if r["mode"] == "reward"]

combined = []
for r in triangle:
    combined.append({
        "protocol": "triangle", "seed": r["seed"],
        "init_mae": r["init_eval_mae"], "final_mae": r["final_eval_mae"],
        "delta_mae": r["delta_mae"], "improved": r["improved"],
        "init_arrivals": r["init_eval_arrivals"], "final_arrivals": r["final_eval_arrivals"],
    })
for r in wrap:
    combined.append({
        "protocol": "wrap", "seed": r["seed"],
        "init_mae": r["init_eval_mae"], "final_mae": r["final_eval_mae"],
        "delta_mae": r["delta_mae"], "improved": r["improved"],
        "init_arrivals": r["init_eval_arrivals"], "final_arrivals": r["final_eval_arrivals"],
    })

with open(RESULTS / "tracking_protocols_by_seed.csv", "w", newline="", encoding="utf-8") as f:
    writer = csv.DictWriter(f, fieldnames=list(combined[0].keys()))
    writer.writeheader()
    writer.writerows(combined)

agg_rows = []
for protocol in ["triangle", "wrap"]:
    subset = [r for r in combined if r["protocol"] == protocol]
    agg_rows.append({
        "protocol": protocol,
        "n": len(subset),
        "mean_init_mae": sum(float(r["init_mae"]) for r in subset) / len(subset),
        "mean_final_mae": sum(float(r["final_mae"]) for r in subset) / len(subset),
        "mean_delta_mae": sum(float(r["delta_mae"]) for r in subset) / len(subset),
        "n_improved": sum(int(r["improved"]) for r in subset),
        "mean_init_arrivals": sum(int(r["init_arrivals"]) for r in subset) / len(subset),
        "mean_final_arrivals": sum(int(r["final_arrivals"]) for r in subset) / len(subset),
    })

with open(RESULTS / "tracking_protocols_aggregate.csv", "w", newline="", encoding="utf-8") as f:
    writer = csv.DictWriter(f, fieldnames=list(agg_rows[0].keys()))
    writer.writeheader()
    writer.writerows(agg_rows)

fig, axes = plt.subplots(1, 2, figsize=(11, 4.2), sharey=True)
width = 0.38
for ax, protocol in zip(axes, ["triangle", "wrap"]):
    subset = [r for r in combined if r["protocol"] == protocol]
    x = list(range(len(subset)))
    ax.bar([i - width / 2 for i in x], [float(r["init_mae"]) for r in subset], width, label="initial", color="#999999")
    ax.bar([i + width / 2 for i in x], [float(r["final_mae"]) for r in subset], width, label="reward-trained", color="#1f77b4")
    ax.set_xticks(x); ax.set_xticklabels([r["seed"] for r in subset], rotation=30)
    ax.set_title(f"Online tracking, {protocol}: MAE {sum(int(r['improved']) for r in subset)}/6 improved")
    ax.set_ylabel("moving-cue MAE")
    ax.grid(axis="y", alpha=0.3)
    ax.legend(fontsize=8)
fig.tight_layout()
fig.savefig(RESULTS / "tracking_protocol_compare.png", dpi=160)
plt.close(fig)

print("wrote", RESULTS / "tracking_wrap_per_seed.csv")
print("wrote", RESULTS / "tracking_wrap_aggregate.csv")
print("wrote", RESULTS / "tracking_protocols_by_seed.csv")
print("wrote", RESULTS / "tracking_protocols_aggregate.csv")
print("wrote", RESULTS / "tracking_protocol_compare.png")
for row in agg_rows:
    print(row)
