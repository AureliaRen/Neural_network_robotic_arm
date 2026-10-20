"""TCP action server for the ROKAE arm: left / right / stop.

This is the joint-1 controller, matching simple.py / bionic_arm_controller.py:
  left  -> joint 1 += step_rad
  right -> joint 1 -= step_rad
  stop  -> no motion (does not interrupt a move already in progress)

The existing servicer.py is an end-effector yaw controller and rejects -1.
Do not run both servers on the same port.

Protocol (UTF-8, one command per line):
  -1 | left   -> left
   0 | stop   -> stop
   1 | right  -> right
Reply: one line, "OK <detail>" or "ERR <reason>".

Safety: the default is dry-run. Pass --real to connect to the arm.
"""
import argparse
import os
import socket
import threading

import setup_path
import platform

if platform.system() == "Windows":
    from Release.windows import xCoreSDK_python
elif platform.system() == "Linux":
    from Release.linux import xCoreSDK_python
else:
    raise ImportError("Unsupported operating system")

import init
import move_example as example


ACTIONS = {
    "-1": "left",
    "left": "left",
    "0": "stop",
    "stop": "stop",
    "1": "right",
    "right": "right",
}

DEFAULT_IP = os.environ.get("ARM_IP", "192.168.0.160")
DEFAULT_HOST = os.environ.get("ARM_HOST", "0.0.0.0")
DEFAULT_PORT = int(os.environ.get("ARM_PORT", "9000"))
DEFAULT_STEP = float(os.environ.get("ARM_STEP_RAD", "0.1"))
DEFAULT_SPEED = float(os.environ.get("ARM_SPEED", "50"))


def parse_action(message):
    return ACTIONS.get(message.strip().lower())


class ArmController:
    def __init__(self, dry_run=True, home=False, step_rad=DEFAULT_STEP, speed=DEFAULT_SPEED):
        self.dry_run = dry_run
        self.step_rad = float(step_rad)
        self.speed = float(speed)
        self.robot = None
        self.ec = {}
        self.lock = threading.Lock()
        if not self.dry_run:
            self.robot = xCoreSDK_python.xMateRobot(DEFAULT_IP)
            init.pre_op(self.robot, self.ec)
            if home:
                init.pos_init(self.robot, self.ec)

    def apply(self, action):
        with self.lock:
            if action == "stop":
                return "stopped (no motion)"
            delta = self.step_rad if action == "left" else -self.step_rad
            if self.dry_run:
                return "dry-run %s delta=%+.4f rad" % (action, delta)

            joint_pos = self.robot.jointPos(self.ec)
            target_pos = list(joint_pos)
            target_pos[0] += delta

            absjcmd1 = xCoreSDK_python.MoveAbsJCommand(joint_pos, self.speed, 10)
            absjcmd2 = xCoreSDK_python.MoveAbsJCommand(target_pos, self.speed, 10)
            cmd_id = xCoreSDK_python.PyString()
            self.robot.moveAppend([absjcmd1, absjcmd2], cmd_id, self.ec)
            self.robot.moveStart(self.ec)
            example.wait_robot(self.robot, self.ec)
            return "moved %s delta=%+.4f rad" % (action, delta)


def handle_client(conn, addr, controller):
    print("client connected: %s" % (addr,), flush=True)
    buffer = b""
    try:
        while True:
            data = conn.recv(1024)
            if not data:
                break
            buffer += data
            while b"\n" in buffer:
                raw, buffer = buffer.split(b"\n", 1)
                message = raw.decode("utf-8", errors="replace").strip()
                if not message:
                    continue
                action = parse_action(message)
                if action is None:
                    reply = "ERR invalid action: %s" % message
                else:
                    try:
                        reply = "OK " + controller.apply(action)
                    except Exception as exc:
                        reply = "ERR %s" % exc
                print("command=%r action=%s reply=%s" % (message, action, reply), flush=True)
                try:
                    conn.sendall((reply + "\n").encode("utf-8"))
                except OSError:
                    return
    except ConnectionResetError:
        pass
    finally:
        conn.close()
        print("client disconnected: %s" % (addr,), flush=True)


def serve(host, port, controller):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server:
        server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server.bind((host, port))
        server.listen()
        actual_host, actual_port = server.getsockname()[:2]
        print("listening on %s:%s (dry_run=%s step=%.4f speed=%.1f)" %
              (actual_host, actual_port, controller.dry_run, controller.step_rad, controller.speed), flush=True)
        while True:
            conn, addr = server.accept()
            threading.Thread(target=handle_client, args=(conn, addr, controller), daemon=True).start()


def main():
    parser = argparse.ArgumentParser(description="ROKAE joint-1 action server (left/right/stop)")
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--dry-run", action="store_true", help="log and ack only (default)")
    mode.add_argument("--real", action="store_true", help="connect and move the real arm")
    parser.add_argument("--home", action="store_true", help="move to init.pos_init pose before serving")
    parser.add_argument("--host", default=DEFAULT_HOST)
    parser.add_argument("--port", type=int, default=DEFAULT_PORT)
    parser.add_argument("--step-rad", type=float, default=DEFAULT_STEP)
    parser.add_argument("--speed", type=float, default=DEFAULT_SPEED)
    args = parser.parse_args()

    controller = ArmController(
        dry_run=not args.real,
        home=args.home,
        step_rad=args.step_rad,
        speed=args.speed,
    )
    try:
        serve(args.host, args.port, controller)
    except KeyboardInterrupt:
        print("server stopped", flush=True)


if __name__ == "__main__":
    main()
