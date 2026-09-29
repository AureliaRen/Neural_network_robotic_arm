#!/usr/bin/env python3
"""
Parameterized piano calibration and control for xMate SR3C.

Coordinate convention
---------------------
1. The robot base bottom and the piano support surface are both defined on z = 0.
2. Middle C (C4, MIDI 60) is the lateral origin of the keyboard, so its center uses y = 0 by default.
3. The distance from the robot base to Middle C along +x is adjustable.
4. The whole keyboard can still be shifted and rotated slightly to match the real setup.

This script is intended for fast on-site calibration:
- change only a few geometry parameters
- verify Middle C / left / right reference keys
- then run single-note or short sequence control

If the real TCP is not the stick tip, use tool tip offsets to compensate.
"""

from __future__ import annotations

import argparse
import math
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable

import numpy as np
import platform
import setup_path

from log import print_log, print_separator


xCoreSDK_python = None
MoveExecution = None


def load_sdk() -> None:
    global xCoreSDK_python, MoveExecution
    if xCoreSDK_python is not None and MoveExecution is not None:
        return

    if platform.system() == "Windows":
        from Release.windows import xCoreSDK_python as _sdk
        from Release.windows.xCoreSDK_python.EventInfoKey import MoveExecution as _move_execution
    elif platform.system() == "Linux":
        from Release.linux import xCoreSDK_python as _sdk
        from Release.linux.xCoreSDK_python.EventInfoKey import MoveExecution as _move_execution
    else:
        raise ImportError("Unsupported operating system")

    xCoreSDK_python = _sdk
    MoveExecution = _move_execution


M_PI = math.pi
MIDI_START = 21
MIDI_END = 108
MIDDLE_C_MIDI = 60
NOTE_NAMES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]
WHITE_PITCH_CLASSES = {0, 2, 4, 5, 7, 9, 11}


@dataclass
class PianoGeometry:
    support_plane_z_m: float = 0.0
    key_surface_above_support_m: float = 0.028
    middle_c_distance_x_m: float = 0.390
    middle_c_y_m: float = 0.0
    keyboard_yaw_deg: float = 0.0
    white_key_spacing_m: float = 0.023
    black_key_forward_offset_m: float = 0.045
    black_key_height_offset_m: float = 0.012


@dataclass
class ToolGeometry:
    # The reference point is the center of the downward-facing circular bottom plane of the white wrist head.
    # Default pose keeps that circular bottom plane parallel to the tabletop, with the stick pointing downward.
    rpy_rad: tuple[float, float, float] = (-M_PI, 0.0, -M_PI)
    # Offset from that bottom-plane reference to the actual stick tip for the under-face horizontal mount.
    tip_offset_local_m: tuple[float, float, float] = (0.0, 0.0, -0.287)
    hover_height_m: float = 0.045
    press_depth_m: float = 0.010


@dataclass
class MotionProfile:
    default_speed: int = 150
    default_zone: int = 0
    travel_speed: int = 220
    approach_speed: int = 60
    press_speed: int = 25
    home_speed: int = 150
    move_absj_acc: int = 1000
    move_line_acc: int = 300


@dataclass
class NoteEvent:
    midi: int
    hold_s: float


def midi_to_name(midi: int) -> str:
    return f"{NOTE_NAMES[midi % 12]}{midi // 12 - 1}"


def note_name_to_midi(note: str) -> int:
    note = note.strip().upper()
    if len(note) < 2:
        raise ValueError(f"Invalid note name: {note}")

    if note[1] == "#":
        pitch_name = note[:2]
        octave_str = note[2:]
    else:
        pitch_name = note[:1]
        octave_str = note[1:]

    if pitch_name not in NOTE_NAMES:
        raise ValueError(f"Invalid note name: {note}")
    octave = int(octave_str)
    return (octave + 1) * 12 + NOTE_NAMES.index(pitch_name)


def is_white_key(midi: int) -> bool:
    return (midi % 12) in WHITE_PITCH_CLASSES


def white_key_y_positions(spacing_m: float) -> dict[int, float]:
    white_midis = [m for m in range(MIDI_START, MIDI_END + 1) if is_white_key(m)]
    white_index = {midi: idx for idx, midi in enumerate(white_midis)}
    middle_c_index = white_index[MIDDLE_C_MIDI]
    return {
        midi: (idx - middle_c_index) * spacing_m
        for midi, idx in white_index.items()
    }


def rotation_z(theta_rad: float) -> np.ndarray:
    c = math.cos(theta_rad)
    s = math.sin(theta_rad)
    return np.array(
        [
            [c, -s, 0.0],
            [s, c, 0.0],
            [0.0, 0.0, 1.0],
        ],
        dtype=float,
    )


def rotation_xyz(roll: float, pitch: float, yaw: float) -> np.ndarray:
    cr, sr = math.cos(roll), math.sin(roll)
    cp, sp = math.cos(pitch), math.sin(pitch)
    cy, sy = math.cos(yaw), math.sin(yaw)

    rx = np.array([[1, 0, 0], [0, cr, -sr], [0, sr, cr]], dtype=float)
    ry = np.array([[cp, 0, sp], [0, 1, 0], [-sp, 0, cp]], dtype=float)
    rz = np.array([[cy, -sy, 0], [sy, cy, 0], [0, 0, 1]], dtype=float)
    return rz @ ry @ rx


def build_key_centers(geometry: PianoGeometry) -> dict[int, np.ndarray]:
    white_y = white_key_y_positions(geometry.white_key_spacing_m)
    yaw = math.radians(geometry.keyboard_yaw_deg)
    rot = rotation_z(yaw)
    base = np.array(
        [
            geometry.middle_c_distance_x_m,
            geometry.middle_c_y_m,
            geometry.support_plane_z_m + geometry.key_surface_above_support_m,
        ],
        dtype=float,
    )

    centers: dict[int, np.ndarray] = {}
    for midi in range(MIDI_START, MIDI_END + 1):
        if is_white_key(midi):
            local = np.array([0.0, white_y[midi], 0.0], dtype=float)
        else:
            prev_white = midi - 1
            next_white = midi + 1
            local = np.array(
                [
                    geometry.black_key_forward_offset_m,
                    0.5 * (white_y[prev_white] + white_y[next_white]),
                    geometry.black_key_height_offset_m,
                ],
                dtype=float,
            )
        centers[midi] = base + rot @ local
    return centers


def desired_tip_pose(
    midi: int,
    key_centers: dict[int, np.ndarray],
    tool: ToolGeometry,
    pressed: bool,
) -> list[float]:
    center = key_centers[midi].copy()
    center[2] += -tool.press_depth_m if pressed else tool.hover_height_m
    return [center[0], center[1], center[2], *tool.rpy_rad]


def tip_pose_to_end_pose(tip_pose: Iterable[float], tool: ToolGeometry) -> list[float]:
    tip_pose = list(tip_pose)
    pos = np.array(tip_pose[:3], dtype=float)
    roll, pitch, yaw = tip_pose[3:6]
    rot = rotation_xyz(roll, pitch, yaw)
    offset = np.array(tool.tip_offset_local_m, dtype=float)
    end_pos = pos - rot @ offset
    return [end_pos[0], end_pos[1], end_pos[2], roll, pitch, yaw]


def wait_robot(robot, ec) -> None:
    print_separator("wait_robot", length=80)
    while True:
        time.sleep(0.1)
        st = robot.operationState(ec)
        if st in (xCoreSDK_python.OperationState.idle, xCoreSDK_python.OperationState.unknown):
            break
    print("move finished")


def print_move_info(info: dict) -> None:
    print_separator("print move info", length=80)
    print(f"{MoveExecution.ID}:{info[MoveExecution.ID]}")
    print(f"{MoveExecution.ReachTarget}:{info[MoveExecution.ReachTarget]}")
    print(f"{MoveExecution.WaypointIndex}:{info[MoveExecution.WaypointIndex]}")
    print(f"{MoveExecution.Error}:{info[MoveExecution.Error]}")
    print(f"{MoveExecution.Remark}:{info[MoveExecution.Remark]}")


def get_move_info(robot, ec):
    info = robot.queryEventInfo(xCoreSDK_python.Event.moveExecution, ec)
    print_log("queryEventInfo", ec)
    return info


def initialize_robot(robot_ip: str, motion: MotionProfile):
    load_sdk()
    robot = xCoreSDK_python.xMateRobot(robot_ip)
    ec = {}
    print_separator("initialize_robot", length=110)
    robot.setOperateMode(xCoreSDK_python.OperateMode.manual, ec)
    print_log("setOperateMode", ec)
    robot.setPowerState(True, ec)
    print_log("setPowerState", ec)
    robot.setMotionControlMode(xCoreSDK_python.MotionControlMode.NrtCommandMode, ec)
    print_log("setMotionControlMode", ec)
    robot.setDefaultZone(motion.default_zone, ec)
    print_log("setDefaultZone", ec)
    robot.setDefaultSpeed(motion.default_speed, ec)
    print_log("setDefaultSpeed", ec)
    robot.setEventWatcher(xCoreSDK_python.Event.moveExecution, print_move_info, ec)
    return robot, ec


def move_absj_to_pose(robot, ec, pose6: list[float], motion: MotionProfile) -> None:
    cart_pos = xCoreSDK_python.CartesianPosition(pose6)
    joint_pos = robot.model().calcIk(cart_pos, ec)
    print_log("calcIk", ec, ",".join(map(str, joint_pos)))
    cmd = xCoreSDK_python.MoveAbsJCommand(joint_pos, motion.move_absj_acc, motion.default_zone)
    cmd_id = xCoreSDK_python.PyString()
    robot.moveAppend([cmd], cmd_id, ec)
    print_log("moveAppend", ec)
    robot.moveStart(ec)
    print_log("moveStart", ec)
    wait_robot(robot, ec)


def move_linear(robot, ec, pose6: list[float], speed: int, zone: int, motion: MotionProfile) -> None:
    cart_pos = xCoreSDK_python.CartesianPosition(pose6)
    cmd = xCoreSDK_python.MoveLCommand(cart_pos, speed, zone)
    cmd_id = xCoreSDK_python.PyString()
    robot.moveAppend([cmd], cmd_id, ec)
    print_log("moveAppend", ec)
    robot.moveStart(ec)
    print_log("moveStart", ec)
    wait_robot(robot, ec)


def parse_sequence_arg(sequence_text: str) -> list[NoteEvent]:
    events: list[NoteEvent] = []
    if not sequence_text:
        return events

    for chunk in sequence_text.split(","):
        chunk = chunk.strip()
        if not chunk:
            continue
        if ":" in chunk:
            note_text, hold_text = chunk.split(":", 1)
            hold_s = float(hold_text)
        else:
            note_text = chunk
            hold_s = 0.30

        midi = int(note_text) if note_text.lstrip("-").isdigit() else note_name_to_midi(note_text)
        events.append(NoteEvent(midi=midi, hold_s=hold_s))
    return events


def default_calibration_events() -> list[NoteEvent]:
    return [
        NoteEvent(MIDDLE_C_MIDI, 0.20),
        NoteEvent(MIDDLE_C_MIDI - 12, 0.20),
        NoteEvent(MIDDLE_C_MIDI + 12, 0.20),
        NoteEvent(MIDDLE_C_MIDI, 0.20),
    ]


def print_reference_points(key_centers: dict[int, np.ndarray]) -> None:
    print_separator("reference_points", length=110)
    for midi in [21, 48, 60, 72, 84, 108]:
        pos = key_centers[midi]
        print(
            f"{midi_to_name(midi):>3} (MIDI {midi}): "
            f"x={pos[0]:.4f}, y={pos[1]:.4f}, z={pos[2]:.4f}"
        )


def play_sequence(
    robot,
    ec,
    events: list[NoteEvent],
    key_centers: dict[int, np.ndarray],
    tool: ToolGeometry,
    motion: MotionProfile,
) -> None:
    if not events:
        raise ValueError("Sequence is empty.")

    first_hover = tip_pose_to_end_pose(
        desired_tip_pose(events[0].midi, key_centers, tool, pressed=False),
        tool,
    )
    print_separator("move_to_first_hover", length=110)
    move_absj_to_pose(robot, ec, first_hover, motion)

    for idx, event in enumerate(events, start=1):
        if event.midi < MIDI_START or event.midi > MIDI_END:
            raise ValueError(f"MIDI note out of range: {event.midi}")

        note_name = midi_to_name(event.midi)
        print_separator(f"note_{idx}_{note_name}", length=110)
        hover_pose = tip_pose_to_end_pose(
            desired_tip_pose(event.midi, key_centers, tool, pressed=False),
            tool,
        )
        press_pose = tip_pose_to_end_pose(
            desired_tip_pose(event.midi, key_centers, tool, pressed=True),
            tool,
        )

        print(
            f"Target {note_name}: "
            f"hover=({hover_pose[0]:.4f}, {hover_pose[1]:.4f}, {hover_pose[2]:.4f}), "
            f"press=({press_pose[0]:.4f}, {press_pose[1]:.4f}, {press_pose[2]:.4f}), "
            f"hold={event.hold_s:.3f}s"
        )

        move_linear(robot, ec, hover_pose, motion.travel_speed, motion.default_zone, motion)
        move_linear(robot, ec, press_pose, motion.press_speed, 0, motion)
        time.sleep(event.hold_s)
        move_linear(robot, ec, hover_pose, motion.approach_speed, 0, motion)


def build_argument_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Parameterized xMate SR3C piano controller.")
    parser.add_argument("--ip", default="192.168.0.160", help="Robot IP address.")
    parser.add_argument(
        "--mode",
        choices=["calibrate", "single", "sequence"],
        default="calibrate",
        help="Calibration mode moves to a few reference keys. Single mode uses --note. Sequence mode uses --sequence.",
    )
    parser.add_argument("--note", default="C4", help="Single note to play in single mode, e.g. C4 or 60.")
    parser.add_argument(
        "--sequence",
        default="C4:0.30,E4:0.30,G4:0.40,C5:0.50",
        help="Comma-separated notes with optional hold time, e.g. C4:0.3,E4:0.3,G4:0.5.",
    )
    parser.add_argument("--middle-c-distance-m", type=float, default=0.390, help="Robot base to Middle C distance along +x.")
    parser.add_argument("--middle-c-y-m", type=float, default=0.0, help="Lateral offset of Middle C center.")
    parser.add_argument("--keyboard-yaw-deg", type=float, default=0.0, help="Keyboard yaw around z.")
    parser.add_argument("--support-plane-z-m", type=float, default=0.0, help="Support plane height. Robot base bottom and piano support plane are both usually 0.")
    parser.add_argument("--key-surface-above-support-m", type=float, default=0.028, help="Key top height above support plane.")
    parser.add_argument("--hover-height-m", type=float, default=0.045, help="Tip hover height above key surface.")
    parser.add_argument("--press-depth-m", type=float, default=0.010, help="Tip press depth below key surface reference.")
    parser.add_argument("--keyboard-spacing-m", type=float, default=0.023, help="White-key center spacing.")
    parser.add_argument("--black-key-forward-offset-m", type=float, default=0.045, help="Black-key center x offset relative to white-key centers.")
    parser.add_argument("--black-key-height-offset-m", type=float, default=0.012, help="Black-key top height above white-key surface.")
    parser.add_argument("--tip-offset-x-m", type=float, default=0.0, help="Tool tip local x offset from the downward circular face center.")
    parser.add_argument("--tip-offset-y-m", type=float, default=0.0, help="Tool tip local y offset from default TCP.")
    parser.add_argument("--tip-offset-z-m", type=float, default=-0.287, help="Tool tip local z offset from the downward circular face center.")
    parser.add_argument("--tool-roll-rad", type=float, default=-M_PI, help="Tool roll.")
    parser.add_argument("--tool-pitch-rad", type=float, default=0.0, help="Tool pitch.")
    parser.add_argument("--tool-yaw-rad", type=float, default=-M_PI, help="Tool yaw.")
    parser.add_argument("--travel-speed", type=float, default=220.0, help="Move speed between keys.")
    parser.add_argument("--approach-speed", type=float, default=60.0, help="Approach/retract speed.")
    parser.add_argument("--press-speed", type=float, default=25.0, help="Press speed.")
    parser.add_argument("--dry-run", action="store_true", help="Print computed coordinates without connecting to the robot.")
    return parser


def main() -> None:
    parser = build_argument_parser()
    args = parser.parse_args()

    piano = PianoGeometry(
        support_plane_z_m=args.support_plane_z_m,
        key_surface_above_support_m=args.key_surface_above_support_m,
        middle_c_distance_x_m=args.middle_c_distance_m,
        middle_c_y_m=args.middle_c_y_m,
        keyboard_yaw_deg=args.keyboard_yaw_deg,
        white_key_spacing_m=args.keyboard_spacing_m,
        black_key_forward_offset_m=args.black_key_forward_offset_m,
        black_key_height_offset_m=args.black_key_height_offset_m,
    )
    tool = ToolGeometry(
        rpy_rad=(args.tool_roll_rad, args.tool_pitch_rad, args.tool_yaw_rad),
        tip_offset_local_m=(args.tip_offset_x_m, args.tip_offset_y_m, args.tip_offset_z_m),
        hover_height_m=args.hover_height_m,
        press_depth_m=args.press_depth_m,
    )
    motion = MotionProfile(
        travel_speed=int(args.travel_speed),
        approach_speed=int(args.approach_speed),
        press_speed=int(args.press_speed),
    )

    key_centers = build_key_centers(piano)
    print_reference_points(key_centers)

    if args.mode == "calibrate":
        events = default_calibration_events()
    elif args.mode == "single":
        midi = int(args.note) if args.note.lstrip("-").isdigit() else note_name_to_midi(args.note)
        events = [NoteEvent(midi=midi, hold_s=0.30)]
    else:
        events = parse_sequence_arg(args.sequence)

    if args.dry_run:
        print_separator("dry_run_targets", length=110)
        for event in events:
            hover_pose = tip_pose_to_end_pose(
                desired_tip_pose(event.midi, key_centers, tool, pressed=False),
                tool,
            )
            press_pose = tip_pose_to_end_pose(
                desired_tip_pose(event.midi, key_centers, tool, pressed=True),
                tool,
            )
            print(
                f"{midi_to_name(event.midi)}: "
                f"hover=({hover_pose[0]:.4f}, {hover_pose[1]:.4f}, {hover_pose[2]:.4f}), "
                f"press=({press_pose[0]:.4f}, {press_pose[1]:.4f}, {press_pose[2]:.4f}), "
                f"hold={event.hold_s:.3f}s"
            )
        return

    robot = None
    ec = None
    try:
        load_sdk()
        robot, ec = initialize_robot(args.ip, motion)
        play_sequence(robot, ec, events, key_centers, tool, motion)
        print_move_info(get_move_info(robot, ec))
    except Exception as exc:
        print(f"An error occurred: {exc}")
        raise
    finally:
        if robot is not None and ec is not None:
            robot.setNoneEventWatcher(xCoreSDK_python.Event.moveExecution, ec)
            robot.disconnectFromRobot(ec)


if __name__ == "__main__":
    main()
