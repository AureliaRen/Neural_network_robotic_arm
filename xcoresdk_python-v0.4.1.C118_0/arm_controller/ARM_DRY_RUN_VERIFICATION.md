# Dry-run verification (this PC, no real arm)

Date: 2026-10-18. This PC was **not** connected to the arm. The xCore SDK imports
on Windows/Python 3.12, and the TCP action path was tested in dry-run only.

## Mapping verified

| SNN action | Value | Wire code | Arm action | Ack |
|---|---:|---:|---|---|
| TURN_LEFT | 0 | -1 | joint 1 +0.1 rad | `OK dry-run left delta=+0.1000 rad` |
| TURN_RIGHT | 1 | 1 | joint 1 -0.1 rad | `OK dry-run right delta=-0.1000 rad` |
| NO_DECISION | 2 | 0 | no motion | `OK stopped (no motion)` |

## Tests

Synthetic sequence:

```
0,1,2  ->  -1,1,0  ->  left, right, stop  (all OK)
```

Real SNN output: first 5 actions of
`fig4_reproduction/runs/tracking_train_reward_910001/trajectory.csv`
were all `action=0` (TURN_LEFT), sent as `-1`; all five were acked.

Server log showed `listening on 0.0.0.0:19001 (dry_run=True step=0.1000 speed=50.0)`,
clean client connect/disconnect, and no errors. The server was stopped manually
afterward.

## Not verified here

- Real joint motion, direction sign, speed, and workspace limits.
- `stop` interrupting an in-flight move (currently it is a no-op).
- C++ `fig4_closed_loop` socket sink (the Python bridge was used for this test).

The arm classmate should repeat the dry-run, then use
`--real --home --step-rad 0.02 --speed 20` with the lab operator and the e-stop.
