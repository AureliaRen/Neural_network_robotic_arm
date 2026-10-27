# 100-cycle vs 300-cycle reward training

Purpose: test whether a shorter reward-training duration removes the weight-bound
saturation seen at 300 cycles and brings the Fig.12 delay profile closer to the
paper, without changing the network, learning rule, learning rate, or evaluation.

Protocol: holdout seeds `730001-730006`; same paper flags as the 300-cycle holdout
(`psp_history=sum-arrival`, `feedback_topology=bank`, `pre_clock=emission`,
`decision_ms=200`, `feedback_ms=2000`, `noise_sampling=per-phase`,
`feedback_noise=off`, `center_cue=previous`, `center_action=compete`,
`i_rs=8.8779296875`, `w_max=2`, `tau_c=5`). Only `--cycles` changed (100 vs 300).
Static evaluation uses eval seeds `830001-830006`; moving triangle evaluation uses
eval seeds `930001-930006`.

| seed | static 100 | static 300 | triangle MAE 100 | triangle MAE 300 | weights at bound 100 | weights at bound 300 | Fig.12 diff 100 | Fig.12 diff 300 |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 730001 | 0.77 | 1.00 | 3.974 | 1.376 | 0 | 0 | -0.084 | -0.902 |
| 730002 | 0.93 | 1.00 | 3.438 | 0.844 | 0 | 396 | -0.218 | -1.999 |
| 730003 | 0.71 | 0.83 | 6.292 | 8.852 | 0 | 0 | +0.011 | +0.068 |
| 730004 | 0.83 | 1.00 | 5.382 | 1.060 | 0 | 196 | +0.124 | +1.823 |
| 730005 | 0.74 | 0.99 | 6.768 | 2.276 | 0 | 0 | -0.135 | -0.571 |
| 730006 | 0.87 | 1.00 | 5.216 | 0.988 | 0 | 213 | +0.199 | +1.771 |
| mean | 0.808 | 0.970 | 5.178 | 2.566 | 0 | 134.2 | abs 0.128 | abs 1.189 |

`Fig.12 diff` is the active-unit mean of `w(RS->TurnRight) - w(RS->TurnLeft)`
across delays 1-100 ms. The sign depends on the cue side: left-cue seeds
(730001/730002/730005) are negative, right-cue seeds (730003/730004/730006)
positive. The paper's Fig.12B is a right-cue, post-reward panel with a graded
positive difference of roughly 0.1-0.2.

## Reading the table

- **100 cycles removes saturation.** All 400 plastic weights stay inside `(0, 2)`;
  the Fig.12 absolute mean difference is 0.128, in the paper-like range. Static
  approach is lower (mean 0.808 vs 0.970) and moving triangle MAE is worse
  (5.178 vs 2.566), but every seed still beats the untrained moving baseline
  (initial mean MAE 6.894).
- **300 cycles gives stronger behavior but saturates.** The Fig.12 difference grows
  to absolute mean 1.189 because many weights hit the clamp; this is not the
  paper's graded profile.
- **Seed 730003 is a convergence failure at 100 cycles.** It records 0 target
  arrivals (0 learning blocks), so its 100-cycle static score 0.71 is essentially
  the untrained initial policy (0.69). The 200-cycle attempt also had 0 arrivals.
  Only the 300-cycle run learns (0.83 static), but its moving triangle MAE is
  8.852, worse than the untrained 6.292. This seed exposes the missing stopping
  rule: the paper reports convergence in 100-300 steps (Fig.11) but does not
  define a step or a convergence criterion. A fixed duration cannot fit all seeds.
- **Actual learning at 100 cycles is 5/6, not 6/6.** The automated summary's
  `pilot_static_improved` counts `approach > 0.5`, which is not a comparison to
  the seed's initial score; do not quote it as 6/6 learning.

## Conclusion

Shorter training is a genuine fidelity improvement for the Fig.12 mechanism, but
it trades away behavioral strength and does not fix the slow seed. The paper does
not specify `w_max`, a convergence criterion, or a stopping rule, so neither 100
nor 300 cycles is paper-exact. Report both; do not claim that either is the
paper's protocol.

Files: `results/pilot100_by_seed.csv`, `results/pilot100_summary.csv`,
`results/pilot100_comparison.png`; commands in `docs/run_pilot100.ps1`.
