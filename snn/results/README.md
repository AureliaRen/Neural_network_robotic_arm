# Fig.4 DA-STDP reproduction: results and run notes

Date: 2026-10-18. All work is in `fig4_reproduction/`; this run made no writes or git
operations in `net_package-main`. That repo already had uncommitted earlier-phase changes
(tutorials, DASTDP, Fig.4) before this run.
No ZIP was produced (per instruction). This is a conditional behavioral reproduction, not an exact numerical reproduction of the paper.

## What was run

1. **Static two-unit Fig.4 network** (reward / punishment), pre-registered holdout seeds `730001-730006`.
   Source: frozen holdout tree in `_archive/fig4_runs/fig4_independent_holdout_validation_20261017`.
   The independent build of the same training source is in `code/train`.
2. **Online moving cue (Fig.13-style)**, new seeds `910001-910006`, reward and punishment.
   Source: `code/moving` (the moving-tree source, extended to allow learning while the cue moves).
   Two protocols: `triangle` (18 cue levels, up then down) and `wrap` (9 levels, cyclic).
3. **Forward experiment**: the moving-cue training is the new part. The prior holdout only
   evaluated static-trained weights on a moving cue. Here the cue moves every 20 cycles and
   reward/punishment is delivered after the arm reaches the current cue; learning continues online.

## Headline results

### Static holdout (seeds 730001-730006, pre-registered, frozen)

| endpoint | initial | final | improved |
|---|---:|---:|---:|
| reward approach success | 0.650 | 0.970 | 6/6 |
| punishment strict avoidance | 0.2617 | 0.5017 | 6/6 |
| punishment away-action fraction | 0.5032 | 0.5730 | 6/6 |
| moving triangle MAE (static weights) | 6.894 | 2.566 | 5/6 |
| moving wrap MAE (static weights) | 6.960 | 2.763 | 6/6 |

The earlier selection set (seeds 1001-1006) gave reward `0.6383 -> 0.9500`, strict punishment
`0.2617 -> 0.4967`, away fraction `0.5042 -> 0.5757`, triangle MAE `6.699 -> 2.101`,
wrap MAE `6.504 -> 3.059`. The holdout is the stronger evidence; do not pool the two sets.

### Online moving-cue training (new, seeds 910001-910006)

| protocol | mode | mean MAE init -> final | improved | mean arrivals init -> final |
|---|---|---:|---:|---:|
| triangle | reward | 8.435 -> 0.939 | 6/6 | 18.17 -> 172.33 |
| wrap | reward | 6.530 -> 1.668 | 6/6 | 24.33 -> 147.17 |
| triangle | punishment | 8.435 -> 10.390 | 0/6 | 18.17 -> 3.50 |

Reward training produces clear tracking: the arm follows the moving cue and the window error
drops from roughly 5-20 to about 1 (see `tracking_trajectory_910001.png`). Punishment produces
avoidance, not tracking: arrivals fall and the distance to the cue increases. That is reported
as a negative result, not hidden.

### Fig.12 delay profile (mechanistic check)

Computed from the holdout reward `weights_final.csv` files: for the active unit,
`w(TurnRight) - w(TurnLeft)` vs delay 1-100 ms. The sign is correct for all six seeds
(right cue -> positive, left cue -> negative). Magnitudes are often saturated: right-cue
730004/730006 reach about +1.8, 730002 is about -2.0. Seed 730003 is closest to the paper
(+0.068, 82% of delays positive), but it is also the seed that fails the static moving-triangle
endpoint. The paper's Fig.12B shows a graded, mostly positive ~0.1-0.2 profile; our curves are
flat/saturated, so this is a qualitative mechanism match, not a quantitative reproduction.
Details: `fig12_delay_profile.md`.

### Training-duration check (100 vs 300 cycles)

A 100-cycle reward run on the same six holdout seeds removes the weight-bound saturation and
brings the Fig.12 delay profile into the paper-like ~0.1-0.2 range, but weakens behavior:
static approach mean 0.808 vs 0.970, triangle MAE mean 5.178 vs 2.566. Seed 730003 records no
reward in 100 or 200 cycles, so its 100-cycle score is the untrained policy; it needs 300 cycles
but then fails the moving triangle (8.852). The paper gives a 100-300-step convergence range but
no stopping rule, so this is a documented trade-off, not a paper-exact protocol. See
`pilot100.md`.

## Important caveats

- **Weight saturation.** Reward training drives many of the 400 plastic weights to the project
  clamp `[0, 2]`. Per seed (`weights_at_zero/weights_at_max`): 910001 `200/200`, 910002 `200/200`,
  910003 `199/0`, 910004 `196/0`, 910005 `8/0`, 910006 `200/200`; mean `267/400` at a bound.
  The clamp is an implementation assumption; the learned policy is partly a saturated policy.
  The wrap reward run saturates for 4/6 seeds (910001/2/3/6 have 200 weights at each bound);
  910004 and 910005 do not saturate but still improve.
- **Training duration.** Fixed 100 vs 300 cycles changes both behavior and saturation:
  100 cycles -> 0/400 at bounds, Fig.12 abs mean 0.128, static 0.808, triangle MAE 5.178;
  300 -> mean 134/400 at bounds, Fig.12 1.189, static 0.970, MAE 2.566. Seed 730003 has
  0 arrivals at 100/200 cycles; the paper's convergence range (100-300 steps) is undefined.
  Neither duration is paper-exact.
- **Punishment is modest.** Static strict avoidance ends near 0.50 (not a strong avoidance rate),
  and away-action ends near 0.57. Online punishment does not track. Do not claim the paper's
  strong "learned to avoid" wording.
- **Moving-cue protocol assumptions.** Cue levels and the 20-cycle period are inferred from
  Fig.13 (20 cycles is paper-explicit; the 9/18-level waveforms are a reconstruction). Movement
  is one unit per decision; neural time does not advance during movement. Feedback is delivered
  before the cue advances to the next 20-cycle window.
- **Noise.** Gaussian `sigma=20`, one draw held for each 200 ms decision phase, noise off during
  feedback. This was selected on seeds 1001-1006 and frozen for the holdout; it is not specified
  by the paper.
- **PSP / pairing.** `psp_history=sum-arrival` is a project implementation of the alpha PSP;
  Eq.(5) uses source firing time (paper-literal), and the most recent past post spike is paired.
  The paper does not give the pairing rule.
- **Network scale.** 2 RS, 4 CH, 4 LTS; 100 synapses per input-output pair with delays 1-100 ms;
  feedback bank 100 synapses per CH<->LTS pair, weights summing to 30/40 with 10% jitter.
  The feedback weight sums are an assumption.
- **No author code/data.** The paper reports no success rate or raw trajectories; all endpoints
  here are project-defined proxies.
- **Fig.12 magnitude.** The delay-profile sign is correct for all six holdout seeds, but the
  curves are flat/saturated rather than the paper's graded ~0.1-0.2. Do not claim a quantitative
  Fig.12 reproduction.

## Reproduce

Build (Ninja + MSVC; `vcvars64.bat` is required or CMake cannot find `iostream`):

```powershell
cd fig4_reproduction\code\moving
cmake -S . -B build_ninja -G Ninja -DCMAKE_BUILD_TYPE=Release `
  -DSNN_COMPUTE_BACKEND=CPU -DSNN_BUILD_ALL_MAINS=OFF `
  -DSNN_DEFAULT_MAIN=fig4_closed_loop -DSNN_ENABLE_ZMQ=OFF `
  -DSNN_ENABLE_PINOCCHIO=OFF -DSNN_BOOST_INCLUDE_DIR=C:/dev/boost_1_91_0
cmd /c 'call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul && cmake --build build_ninja --target fig4_closed_loop -j 8'
```

The same build commands work in `code\train` (static training; that tree has no `--tracking-cue`).

Run the training/eval commands below from `fig4_reproduction`:

Training, one reward seed, triangle protocol:

```powershell
.\code\moving\build_ninja\fig4_closed_loop.exe `
  --mode reward --seed 910001 --tracking-cue triangle `
  --psp-history sum-arrival --feedback-topology bank --feedback-scale 1 `
  --pre-clock emission --i-rs 8.8779296875 --decision-ms 200 `
  --feedback-noise off --noise-sampling per-phase `
  --center-cue previous --center-action compete `
  --out runs\tracking_train_reward_910001
```

Fixed-weight evaluation of the trained weights:

```powershell
.\code\moving\build_ninja\fig4_closed_loop.exe `
  --mode control --tracking-cue triangle `
  --eval runs\tracking_train_reward_910001\weights_final.csv --eval-seed 4242 `
  --psp-history sum-arrival --feedback-topology bank --feedback-scale 1 `
  --pre-clock emission --i-rs 8.8779296875 --decision-ms 200 `
  --feedback-noise off --noise-sampling per-phase `
  --center-cue previous --center-action compete `
  --out runs\tracking_eval_final_reward_910001
```

Batch scripts used for all seeds:

```powershell
powershell -ExecutionPolicy Bypass -File docs\run_tracking_train.ps1   # 6 seeds, reward + punishment, triangle
powershell -ExecutionPolicy Bypass -File docs\run_tracking_eval.ps1    # init + final evals
powershell -ExecutionPolicy Bypass -File docs\run_tracking_wrap.ps1    # 6 seeds, reward, wrap
```

Analysis:

```powershell
python analysis\tracking_analysis.py
python analysis\tracking_wrap_analysis.py
python analysis\fig4_summary.py
```

## Files

- `results/tracking_per_seed.csv`, `tracking_aggregate.csv`: online triangle, reward/punishment.
- `results/tracking_wrap_per_seed.csv`, `tracking_wrap_aggregate.csv`: online wrap.
- `results/tracking_protocols_by_seed.csv`, `tracking_protocols_aggregate.csv`: triangle + wrap.
- `results/data/static_holdout_by_seed.csv`, `punishment_away_by_seed.csv`,
  `moving_holdout_by_seed.csv`: copied frozen-holdout tables.
- `results/tracking_mae.png`, `tracking_trajectory_910001.png`, `tracking_protocol_compare.png`,
  `fig4_combined_summary.png`, `fig4_moving_static.png`.
- `results/fig12_delay_profile.md`, `fig12_delay_profile_by_seed.csv`,
  `fig12_delay_profile_summary.csv`, `fig12_delay_profile.png`: Fig.12-style delay profile.
- `results/pilot100.md`, `pilot100_by_seed.csv`, `pilot100_summary.csv`,
  `pilot100_comparison.png`: 100-cycle vs 300-cycle training-duration check.
- `runs/tracking_train_*`, `runs/tracking_eval_*`, `runs/tracking_wrap_*`: raw outputs.
- `runs/pilot100_reward_*`, `runs/pilot100_eval6_static_*`, `runs/pilot100_eval6_triangle_*`:
  100-cycle raw outputs.
- `runs/tracking_train_jobs.csv`, `runs/tracking_eval_jobs.csv`, `runs/tracking_wrap_jobs.csv`,
  `runs/pilot100_train_jobs.csv`, `runs/pilot100_static_jobs.csv`,
  `runs/pilot100_triangle_jobs.csv`: job logs (all exit 0).

## Hashes (SHA-256)

| file | sha256 |
|---|---|
| `code/train/build_ninja/fig4_closed_loop.exe` | `DAD000044FA1F7553078A54DD4848A73168C46DCC1228B18044875FE94936840` |
| `code/moving/build_ninja/fig4_closed_loop.exe` | `D6B63AFA0C21C42B4DAEB6581033723782415B9044AD5A1B45BB144DCBEB9B45` |
| `code/train/source_file_realtime_v1_async/mainprogram/fig4_closed_loop.cpp` | `ACC35C53B033A4AF538B7FAD8825873DB3C7E686122E60D56D5600CCD40283AB` |
| `code/moving/source_file_realtime_v1_async/mainprogram/fig4_closed_loop.cpp` | `19DF9078E716D44E687F94A2438D991EA188B82BF78EDE633EFC29D21DAE07E3` |
| `code/moving/source_file_realtime_v1_async/LearningRule/src/Rule/DASTDP.cpp` | `07C487E1542E34CB54794726C4D072326D4D9AD5F5B67AF27947F7AE8397B298` |
| `_archive/fig4_runs/fig4_independent_holdout_validation_20261017/FROZEN_PROTOCOL.json` | `928D84824F1B9E671E8BA9375EA7FF4FE7019F55EDD198F3A68BDDBB8BA830DC` |

## Failure log

- `cmake --build` without `vcvars64.bat` failed with C1083 (`iostream` not found); the build
  was rerun inside the MSVC environment. No code change was needed.
- Early PowerShell scripts used a hard-coded Chinese path and hit UTF-8/GBK mojibake; they now
  use `$PSScriptRoot`.
- The static moving holdout triangle endpoint worsened for seed `730003` (6.308 -> 8.852);
  the aggregate still improved 5/6. Retained as a real failure.
- Online punishment improved 0/6 on tracking MAE; the arm avoids the cue instead of following
  it. This is the expected effect of negative feedback but is not Fig.13 tracking; retained.
- Seed `730003` had 0 target arrivals in the 100-cycle and 200-cycle reward runs, so the
  100-cycle score is the untrained policy; only 300 cycles learns, but its moving triangle MAE
  is worse (8.852). The paper does not define a convergence/stopping rule; retained.
- The first `run_pilot100.ps1` used `$args` as a `Start-Job` parameter and silently printed
  `done` without running the new jobs. It was rewritten to run sequentially, check
  `$LASTEXITCODE`, and write per-run logs; all six training and twelve evaluation jobs then
  exited 0.
- No seed replacement, no parameter sweep, no eligibility freeze, no action gate, no selected
  action current. All reported seeds are in the CSVs.
