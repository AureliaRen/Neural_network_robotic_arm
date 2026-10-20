"""Send left / right / stop to the arm peer (fire-and-forget).

Wire protocol: one UTF-8 line per command, newline terminated:

    left  / -1 -> -1
    right /  1 ->  1
    stop  /  0 ->  0

The peer (the arm classmate's servicer.py) does not send an acknowledgement.
This client therefore does not read a reply. A printed "sent" only means the
TCP write succeeded, not that the arm moved or reached a target.
"""
import argparse
import os
import socket


ACTION_CODES = {"left": "-1", "right": "1", "stop": "0"}
ACTION_ALIASES = {
    "left": "left",
    "-1": "left",
    "right": "right",
    "1": "right",
    "stop": "stop",
    "0": "stop",
}
DEFAULT_HOST = os.environ.get("ARM_HOST", "172.30.187.22")
DEFAULT_PORT = int(os.environ.get("ARM_PORT", "9000"))


def normalize_action(value):
    """Accept left/right/stop or -1/1/0; return the canonical word or None."""
    return ACTION_ALIASES.get(str(value).strip().lower())


def send_action(action, host=DEFAULT_HOST, port=DEFAULT_PORT, timeout=5.0):
    """Send one action and return immediately; no response is read."""
    canonical = normalize_action(action)
    if canonical is None:
        raise ValueError("invalid action: %r (use left/-1, right/1, stop/0)" % (action,))
    code = ACTION_CODES[canonical]
    with socket.create_connection((host, port), timeout=timeout) as sock:
        sock.sendall((code + "\n").encode("utf-8"))
    return "sent %s (%s)" % (canonical, code)


def interactive(host, port, timeout=5.0):
    print("connecting to %s:%s" % (host, port), flush=True)
    print("commands: left/-1, right/1, stop/0, q", flush=True)
    with socket.create_connection((host, port), timeout=timeout) as sock:
        while True:
            try:
                text = input("> ").strip().lower()
            except EOFError:
                break
            if text in ("q", "quit", "exit"):
                break
            action = normalize_action(text)
            if action is None:
                print("invalid; use left/-1, right/1, stop/0", flush=True)
                continue
            code = ACTION_CODES[action]
            try:
                sock.sendall((code + "\n").encode("utf-8"))
            except OSError as exc:
                print("send failed: %s" % exc, flush=True)
                break
            print("sent %s (%s)" % (action, code), flush=True)


def main():
    parser = argparse.ArgumentParser(description="Fire-and-forget arm action client")
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--action", help="one action: left/-1, right/1, stop/0")
    group.add_argument("--interactive", action="store_true", help="keep one connection and read commands")
    parser.add_argument("--host", default=DEFAULT_HOST, help="peer IPv4 (default: %s)" % DEFAULT_HOST)
    parser.add_argument("--port", type=int, default=DEFAULT_PORT)
    args = parser.parse_args()

    try:
        if args.interactive:
            interactive(args.host, args.port)
        else:
            action = normalize_action(args.action)
            if action is None:
                parser.error("--action must be left/-1, right/1, or stop/0 (for -1 use --action=-1)")
            print(send_action(action, args.host, args.port), flush=True)
    except OSError as exc:
        print("connection/send failed: %s" % exc, flush=True)
    except ValueError as exc:
        parser.error(str(exc))


if __name__ == "__main__":
    main()
