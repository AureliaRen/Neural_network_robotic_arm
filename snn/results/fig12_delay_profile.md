# Fig.12-style delay-dependent weight profile

Date: 2026-10-18. Inputs: frozen holdout reward runs
`_archive/fig4_runs/fig4_independent_holdout_validation_20261017/runs/training/reward_730001..730006`.

## What was computed

For each seed, `weights_init.csv` and `weights_final.csv` were read. The active
unit is unit 0 for a left starting position (`initial_position <= 0`) and unit 1
for a right starting position. For the active unit:

```
diff(delay) = w(source RS -> CH TurnRight) - w(source RS -> CH TurnLeft)
```

Unit 0 uses source 2 and targets 4 (TurnLeft) / 5 (TurnRight); unit 1 uses
source 3 and targets 6 (TurnLeft) / 7 (TurnRight). Delays are 10..1000 scheduler
steps, i.e. 1..100 ms in 1 ms increments. This follows the paper's Fig.12
quantity: the difference of synaptic strength from the input neuron to the two
output neurons at each delay.

## Per-seed result (active unit)

| seed | start | active unit | mean diff init | mean diff final | positive delays | weights at 0 / max |
|---:|---:|---:|---:|---:|---:|---:|
| 730001 | -4 (left) | 0 | +0.0050 | -0.9022 | 0% | 0 / 0 |
| 730002 | -3 (left) | 0 | -0.0082 | -1.9986 | 0% | 200 / 196 |
| 730003 | +7 (right) | 1 | +0.0109 | +0.0684 | 82% | 0 / 0 |
| 730004 | +7 (right) | 1 | +0.0059 | +1.8235 | 100% | 196 / 0 |
| 730005 | -6 (left) | 0 | -0.0019 | -0.5707 | 0% | 0 / 0 |
| 730006 | +3 (right) | 1 | -0.0011 | +1.7708 | 100% | 200 / 13 |

Right-cue seeds 730003/730004/730006 are directly comparable to the paper's
Fig.12B (cue on the right after reward). Their mean final diff is about +1.2,
but that mean is dominated by 730004 and 730006, which are at the clamp. The
mean initial diff is near zero. Left-cue seeds are the mirror sign: mean final
about -0.9, also with one seed (730002) at the clamp.

## Comparison with paper Fig.12B

The paper's Fig.12B shows, for a cue on the right after reward training, a
mostly positive difference of about +0.1 to +0.2 across delays; before training
it fluctuates around zero. The intended qualitative statement is that reward
strengthens the input->TurnRight path relative to input->TurnLeft.

Our result reproduces the **sign** for all six holdout seeds:
right cue -> positive, left cue -> negative. It does not reproduce the paper's
**magnitude or graded profile**:

- 730003 is closest to the paper (+0.068, 82% of delays positive), but it is
  also the one seed whose static moving-triangle endpoint got worse
  (6.308 -> 8.852).
- 730004 and 730006 reach about +1.8; 730002 is flat at about -2.0. These
  seeds are at the `[0, 2]` weight clamp.
- The curves are roughly flat with small delay-to-delay fluctuations, not the
  paper's graded noisy profile.

This is expected from the saturation documented in `README.md`: reward training
drives a large fraction of the 400 plastic weights to the project clamp
`w_max = 2`, so a graded physiological weight profile cannot emerge. The sign
result supports the qualitative DA-STDP mechanism; the magnitude result is not
a quantitative reproduction of Fig.12 and should not be presented as one.

## Reproduce

From `fig4_reproduction`:

```powershell
python analysis\fig12_delay_profile.py
```

Outputs:

- `results/fig12_delay_profile_by_seed.csv` (all units, all delays)
- `results/fig12_delay_profile_summary.csv` (per-seed mean diff / bounds)
- `results/fig12_delay_profile.png` (right-cue and left-cue panels)
