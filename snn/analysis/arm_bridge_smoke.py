"""Send SNN actions (0/1/2) to the arm peer, fire-and-forget.

Mapping:
  SNN 0 TURN_LEFT  -> wire -1 (left)
  SNN 1 TURN_RIGHT -> wire  1 (right)
  SNN 2 NO_DECISION-> wire  0 (stop)

The peer does not acknowledge; this script does not read a reply.
"""
import argparse
import csv
import socket


ACTION_MAP = {0: "-1", 1: "1", 2: "0"}
DEFAULT_HOST = "172.30.187.22"
DEFAULT_PORT = 9000


def send_action(snn_action, host, port, timeout=5.0):
    code = ACTION_MAP[int(snn_action)]
    with socket.create_connection((host, port), timeout=timeout) as sock:
        sock.sendall((code + "\n").encode("utf-8"))
    return code


def actions_from_csv(path, limit):
    with open(path, newline="", encoding="utf-8") as f:
        rows = list(csv.DictReader(f))
    return [int(row["action"]) for row in rows[:limit]]


def main():
    parser = argparse.ArgumentParser(description="Send SNN actions to the arm (no ack)")
    parser.add_argument("--actions", help="comma-separated SNN actions, e.g. 0,1,2")
    parser.add_argument("--csv", help="trajectory.csv with an action column")
    parser.add_argument("--limit", type=int, default=5)
    parser.add_argument("--host", default=DEFAULT_HOST)
    parser.add_argument("--port", type=int, default=DEFAULT_PORT)
    args = parser.parse_args()

    if args.actions:
        actions = [int(x) for x in args.actions.split(",") if x.strip()]
    elif args.csv:
        actions = actions_from_csv(args.csv, args.limit)
    else:
        parser.error("provide --actions or --csv")

    for snn_action in actions:
        wire = ACTION_MAP.get(int(snn_action))
        if wire is None:
            print("skip invalid SNN action: %r" % snn_action, flush=True)
            continue
        try:
            send_action(snn_action, args.host, args.port)
            print("snn=%d -> arm=%s sent" % (snn_action, wire), flush=True)
        except OSError as exc:
            print("send failed for snn=%d: %s" % (snn_action, exc), flush=True)
            break


if __name__ == "__main__":
    main()
