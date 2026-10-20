"""Send left / right / stop to action_server.py.

Example:
  python action_client.py --action left
  python action_client.py --action stop
  python action_client.py --interactive
"""
import argparse
import socket


ACTION_CODES = {"left": "-1", "right": "1", "stop": "0"}


def send_action(action, host="127.0.0.1", port=9000, timeout=5.0):
    code = ACTION_CODES[action]
    with socket.create_connection((host, port), timeout=timeout) as sock:
        sock.sendall((code + "\n").encode("utf-8"))
        reply = sock.recv(4096).decode("utf-8", errors="replace").strip()
    return reply


def main():
    parser = argparse.ArgumentParser(description="Client for the ROKAE action server")
    parser.add_argument("--action", choices=sorted(ACTION_CODES), help="left, right, or stop")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=9000)
    parser.add_argument("--interactive", action="store_true", help="read commands from stdin")
    args = parser.parse_args()

    if args.interactive:
        print("commands: left / right / stop / q", flush=True)
        while True:
            try:
                text = input("> ").strip().lower()
            except EOFError:
                break
            if text in ("q", "quit", "exit"):
                break
            if text not in ACTION_CODES:
                print("invalid; use left/right/stop", flush=True)
                continue
            print(send_action(text, args.host, args.port), flush=True)
        return

    if not args.action:
        parser.error("--action is required unless --interactive is used")
    print(send_action(args.action, args.host, args.port), flush=True)


if __name__ == "__main__":
    main()
