"""Send left / right / stop to the arm peer (fire-and-forget).

Wire protocol: one UTF-8 line per command, newline terminated:

    left  -> -1
    right ->  1
    stop  ->  0

The peer (the arm classmate's servicer.py) does not send an acknowledgement.
This client therefore does not read a reply. A printed "sent" only means the
TCP write succeeded, not that the arm moved or reached a target.
"""
import argparse
import os
import socket


ACTION_CODES = {"left": "-1", "right": "1", "stop": "0"}
DEFAULT_HOST = os.environ.get("ARM_HOST", "172.30.187.22")
DEFAULT_PORT = int(os.environ.get("ARM_PORT", "9000"))


def send_action(action, host=DEFAULT_HOST, port=DEFAULT_PORT, timeout=5.0):
    """Send one action and return immediately; no response is read."""
    code = ACTION_CODES[action]
    with socket.create_connection((host, port), timeout=timeout) as sock:
        sock.sendall((code + "\n").encode("utf-8"))
    return "sent %s (%s)" % (action, code)


def interactive(host, port, timeout=5.0):
    print("connecting to %s:%s" % (host, port), flush=True)
    print("commands: left / right / stop / q", flush=True)
    with socket.create_connection((host, port), timeout=timeout) as sock:
        while True:
            try:
                text = input("> ").strip().lower()
            except EOFError:
                break
            if text in ("q", "quit", "exit"):
                break
            if text not in ACTION_CODES:
                print("invalid; use left / right / stop", flush=True)
                continue
            try:
                sock.sendall((ACTION_CODES[text] + "\n").encode("utf-8"))
            except OSError as exc:
                print("send failed: %s" % exc, flush=True)
                break
            print("sent %s (%s)" % (text, ACTION_CODES[text]), flush=True)


def main():
    parser = argparse.ArgumentParser(description="Fire-and-forget arm action client")
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--action", choices=sorted(ACTION_CODES), help="send one action")
    group.add_argument("--interactive", action="store_true", help="keep one connection and read commands")
    parser.add_argument("--host", default=DEFAULT_HOST, help="peer IPv4 (default: %s)" % DEFAULT_HOST)
    parser.add_argument("--port", type=int, default=DEFAULT_PORT)
    args = parser.parse_args()

    try:
        if args.interactive:
            interactive(args.host, args.port)
        else:
            print(send_action(args.action, args.host, args.port), flush=True)
    except OSError as exc:
        print("connection/send failed: %s" % exc, flush=True)


if __name__ == "__main__":
    main()
