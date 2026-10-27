# -*- coding: utf-8 -*-
"""da_stdp_analysis.py - plots/summaries for fig4_da_stdp_training.
Expects results/<mode>_seed1/training.csv and results/multi/<mode>_<seed>/training.csv
plus results/reward_seed1/weights_vs_delay.txt (relative to CWD)."""
import glob
import os
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

R = "results"
MODES = ["control", "reward", "punishment"]
COL = {"control": "0.5", "reward": "C2", "punishment": "C3"}

def rolling(s, w=10):
    return pd.Series(s).rolling(w, min_periods=1).mean().to_numpy()

# 1) success curve (seed 1)
plt.figure(figsize=(8, 4.6))
for m in MODES:
    p = f"{R}/{m}_seed1/training.csv"
    if os.path.exists(p):
        d = pd.read_csv(p)
        plt.plot(d["trial"], rolling(d["success"].to_numpy()), color=COL[m], lw=1.6, label=m)
plt.axhline(0.5, ls=":", color="k"); plt.xlabel("trial"); plt.ylabel("success (rolling 10)")
plt.title("success / avoidance rate (seed 1)"); plt.legend(); plt.grid(alpha=.3)
plt.tight_layout(); plt.savefig("success_curve.png", dpi=150); plt.close()

# 2) four group weights (reward seed1)
p = f"{R}/reward_seed1/training.csv"
if os.path.exists(p):
    d = pd.read_csv(p)
    plt.figure(figsize=(8, 4.8))
    for c, lab in [("mean_W_LL", "W_LL"), ("mean_W_LR", "W_LR"),
                   ("mean_W_RL", "W_RL"), ("mean_W_RR", "W_RR")]:
        plt.plot(d["trial"], rolling(d[c].to_numpy(), 10), lw=1.6, label=lab)
    plt.xlabel("trial"); plt.ylabel("mean weight (rolling 10)")
    plt.title("four plastic groups (reward, seed 1)"); plt.legend(); plt.grid(alpha=.3)
    plt.tight_layout(); plt.savefig("weight_groups.png", dpi=150); plt.close()

    plt.figure(figsize=(8, 4.6))
    plt.plot(d["trial"], rolling(d["correct_wrong_gap"].to_numpy(), 10), color="C0", lw=1.8)
    plt.axhline(0, ls=":", color="k"); plt.xlabel("trial"); plt.ylabel("correct - wrong gap")
    plt.title("correct_wrong_gap (reward, seed 1)"); plt.grid(alpha=.3)
    plt.tight_layout(); plt.savefig("correct_wrong_gap.png", dpi=150); plt.close()

# 3) weight vs delay, 4 groups, before/after
p = f"{R}/reward_seed1/weights_vs_delay.txt"
if os.path.exists(p):
    wd = pd.read_csv(p)
    fig, axes = plt.subplots(2, 2, figsize=(12, 8), sharex=True)
    for ax, g in zip(axes.ravel(), ["W_LL", "W_LR", "W_RL", "W_RR"]):
        gd = wd[wd["group"] == g].sort_values("delay_steps")
        ax.scatter(gd["delay_ms"], gd["w_init"], s=10, color="0.6", label="before")
        ax.scatter(gd["delay_ms"], gd["w_final"], s=10, color="C3", label="after")
        ax.set_title(g); ax.grid(alpha=.3); ax.legend(fontsize=8)
    for ax in axes[1]:
        ax.set_xlabel("delay (ms)")
    for ax in axes[:, 0]:
        ax.set_ylabel("weight")
    plt.suptitle("weight vs delay before/after (reward, seed 1)")
    plt.tight_layout(); plt.savefig("weight_vs_delay_before_after.png", dpi=150); plt.close()

# 4) multi-seed
rows = []
for p in glob.glob(f"{R}/multi/*/training.csv"):
    d = pd.read_csv(p)
    if d.empty:
        continue
    n = len(d); third = max(1, n // 3)
    rows.append(dict(mode=d["mode"].iloc[0], seed=int(d["seed"].iloc[0]),
                     first_third=d["success"].iloc[:third].mean(),
                     last_third=d["success"].iloc[-third:].mean(),
                     final=d["success"].mean(),
                     final_W_LL=d["mean_W_LL"].iloc[-1], final_W_LR=d["mean_W_LR"].iloc[-1],
                     final_W_RL=d["mean_W_RL"].iloc[-1], final_W_RR=d["mean_W_RR"].iloc[-1],
                     final_gap=d["correct_wrong_gap"].iloc[-1],
                     no_decision=(d["action"] == "NO_DECISION").mean()))
ms = pd.DataFrame(rows)
if not ms.empty:
    ms.to_csv("multiseed_summary.csv", index=False)
    print(ms.groupby("mode")[["first_third", "last_third", "final", "final_gap", "no_decision"]].agg(["mean", "std"]))
    plt.figure(figsize=(8, 5))
    for m in MODES:
        ss = []
        for p in glob.glob(f"{R}/multi/{m}_*/training.csv"):
            d = pd.read_csv(p)
            if not d.empty:
                ss.append(rolling(d["success"].to_numpy()))
        if not ss:
            continue
        n = min(len(s) for s in ss); M = np.vstack([s[:n] for s in ss])
        mean = M.mean(0); sem = M.std(0) / np.sqrt(M.shape[0]); x = np.arange(n)
        plt.plot(x, mean, color=COL[m], lw=1.8, label="%s (n=%d)" % (m, M.shape[0]))
        plt.fill_between(x, mean - sem, mean + sem, color=COL[m], alpha=0.2)
    plt.axhline(0.5, ls=":", color="k"); plt.xlabel("trial")
    plt.ylabel("success (rolling 10, mean +/- SEM)")
    plt.title("multi-seed learning curves"); plt.legend(); plt.grid(alpha=.3)
    plt.tight_layout(); plt.savefig("multiseed_learning_curve.png", dpi=150); plt.close()
print("plots written")
