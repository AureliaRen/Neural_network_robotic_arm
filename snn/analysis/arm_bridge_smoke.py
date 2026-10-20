"""Smoke-test the SNN -> arm action mapping against a dry-run action server.

SNN action values (Fig4VirtualEnvironment.h):
  0 = TURN_LEFT   -> arm "left"  (wire code -1)
  1 = TURN_RIGHT  -> arm "right" (wire code  1)
  2 = NO_DECISION -> arm "stop"  (wire code  0)

Examples:
  python analysis/arm_bridge_smoke.py --actions 0,1,2 --port 19001
  python analysis/arm_bridge_smoke.py --csv runs/tracking_train_reward_910001/trajectory.csv --limit 5 --port 19001
"""
import argparse
import csv
import socket

ACTION_MAP = {
    0: ("left", "-1"),
    1: ("right", "1"),
    2: ("stop", "0"),
}


def read_trajectory_actions(path, limit):
    rows = []
    with open(path, newline="", encoding="utf-8") as f:
        for row in csv.DictReader(f):
            rows.append((int(row["cycle"]), int(row["action"])))
            if len(rows) >= limit:
                break
    return rows


def main():
    parser = argparse.ArgumentParser(description="SNN action -> arm dry-run server smoke test")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=19001)
    parser.add_argument("--actions", help="comma-separated SNN action values, e.g. 0,1,2")
    parser.add_argument("--csv", help="trajectory.csv with cycle,action columns")
    parser.add_argument("--limit", type=int, default=5)
    args = parser.parse_args()

    if args.csv:
        sequence = read_trajectory_actions(args.csv, args.limit)
    else:
        text = args.actions if args.actions else "0,1,2"
        sequence = [(i, int(value)) for i, value in enumerate(text.split(","))]

    with socket.create_connection((args.host, args.port), timeout=5) as sock:
        for source, value in sequence:
            if value not in ACTION_MAP:
                raise SystemExit("invalid SNN action %r at %r" % (value, source))
            arm_name, wire = ACTION_MAP[value]
            sock.sendall((wire + "\n").encode("utf-8"))
            ack = sock.recv(4096).decode("utf-8", errors="replace").strip()
            print("source=%s snn_action=%d arm=%s wire=%s ack=%s" %
                  (source, value, arm_name, wire, ack), flush=True)


if __name__ == "__main__":
    main()
