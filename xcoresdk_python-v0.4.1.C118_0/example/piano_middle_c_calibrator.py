#!/usr/bin/env python3
"""
Interactive Middle C calibrator for the piano robot setup.

Run dry first:
    python piano_middle_c_calibrator.py --dry-run

Run on robot:
    python piano_middle_c_calibrator.py --ip 192.168.0.160

Commands in the interactive loop:
    x+ / x-      move Middle C along robot x
    y+ / y-      move Middle C along robot y
    z+ / z-      move key surface height
    step 5       set nudge step in mm
    move         move to current hover point
    tap          press Middle C once and lift
    show         print current calibration
    save         save calibration JSON and print code snippet
    quit         exit
"""

from __future__ import annotations

import argparse
import json
import math
from dataclasses import asdict
from pathlib import Path

import setup_path

import piano_sequence_player_windows as player
from piano_calibration_and_control import (
    MIDDLE_C_MIDI,
    MotionProfile,
    PianoGeometry,
    ToolGeometry,
    build_key_centers,
    desired_tip_pose,
    midi_to_name,
    tip_pose_to_end_pose,
)


M_PI = math.pi
EXAMPLE_DIR = Path(__file__).resolve().parent
DEFAULT_OUTPUT = EXAMPLE_DIR / "middle_c_calibration.json"


def build_default_piano() -> PianoGeometry:
    return PianoGeometry(
        support_plane_z_m=0.0,
        key_surface_above_support_m=0.300,
        middle_c_distance_x_m=0.390,
        middle_c_y_m=0.0,
        keyboard_yaw_deg=0.0,
        white_key_spacing_m=0.023,
        black_key_forward_offset_m=0.045,
        black_key_height_offset_m=0.012,
    )


def build_default_tool() -> ToolGeometry:
    return ToolGeometry(
        rpy_rad=(-M_PI, 0.0, -M_PI),
        tip_offset_local_m=(0.0, 0.0, -0.287),
        hover_height_m=0.080,
        press_depth_m=0.012,
    )


def build_default_motion() -> MotionProfile:
    return MotionProfile(
        default_speed=200,
        default_zone=0,
        travel_speed=180,
        approach_speed=80,
        press_speed=30,
        home_speed=200,
        move_absj_acc=1000,
        move_line_acc=500,
    )


def middle_c_poses(piano: PianoGeometry, tool: ToolGeometry) -> tuple[list[float], list[float]]:
    key_centers = build_key_centers(piano)
    hover_pose = tip_pose_to_end_pose(
        desired_tip_pose(MIDDLE_C_MIDI, key_centers, tool, pressed=False),
        tool,
    )
    press_pose = tip_pose_to_end_pose(
        desired_tip_pose(MIDDLE_C_MIDI, key_centers, tool, pressed=True),
        tool,
    )
    return hover_pose, press_pose


def print_current(piano: PianoGeometry, tool: ToolGeometry) -> None:
    key_centers = build_key_centers(piano)
    key = key_centers[MIDDLE_C_MIDI]
    hover_pose, press_pose = middle_c_poses(piano, tool)
    print()
    print(f"{midi_to_name(MIDDLE_C_MIDI)} key center:")
    print(f"  x={key[0]:.4f}, y={key[1]:.4f}, z={key[2]:.4f}")
    print("Calibration values:")
    print(f"  middle_c_distance_x_m={piano.middle_c_distance_x_m:.4f}")
    print(f"  middle_c_y_m={piano.middle_c_y_m:.4f}")
    print(f"  key_surface_above_support_m={piano.key_surface_above_support_m:.4f}")
    print("Robot end poses:")
    print(f"  hover=({hover_pose[0]:.4f}, {hover_pose[1]:.4f}, {hover_pose[2]:.4f})")
    print(f"  press=({press_pose[0]:.4f}, {press_pose[1]:.4f}, {press_pose[2]:.4f})")


def save_calibration(path: Path, piano: PianoGeometry, tool: ToolGeometry) -> None:
    data = {
        "piano": asdict(piano),
        "tool": asdict(tool),
        "note": midi_to_name(MIDDLE_C_MIDI),
        "midi": MIDDLE_C_MIDI,
    }
    path.write_text(json.dumps(data, indent=2), encoding="utf-8")
    print(f"Saved: {path}")
    print("Paste these values into piano_sequence_player_windows.py:")
    print(f"  key_surface_above_support_m={piano.key_surface_above_support_m:.4f},")
    print(f"  middle_c_distance_x_m={piano.middle_c_distance_x_m:.4f},")
    print(f"  middle_c_y_m={piano.middle_c_y_m:.4f},")


def move_to_hover(robot, ec, piano: PianoGeometry, tool: ToolGeometry, motion: MotionProfile) -> None:
    hover_pose, _ = middle_c_poses(piano, tool)
    player.move_l_to_pose(robot, ec, hover_pose, motion.travel_speed, motion.default_zone, "middle_c_hover")


def tap_middle_c(robot, ec, piano: PianoGeometry, tool: ToolGeometry, motion: MotionProfile) -> None:
    hover_pose, press_pose = middle_c_poses(piano, tool)
    player.move_l_to_pose(robot, ec, hover_pose, motion.approach_speed, 0, "middle_c_hover")
    player.move_l_to_pose(robot, ec, press_pose, motion.press_speed, 0, "middle_c_press")
    player.move_l_to_pose(robot, ec, hover_pose, motion.approach_speed, 0, "middle_c_lift")


def interactive_loop(robot, ec, piano: PianoGeometry, tool: ToolGeometry, motion: MotionProfile, output: Path, dry_run: bool) -> None:
    step_m = 0.005
    print_current(piano, tool)
    print()
    print("Commands: x+ x- y+ y- z+ z- | step <mm> | move | tap | show | save | quit")

    while True:
        command = input("middle-c> ").strip().lower()
        if command in ("q", "quit", "exit"):
            return
        if command == "":
            continue

        moved = False
        if command == "x+":
            piano.middle_c_distance_x_m += step_m
            moved = True
        elif command == "x-":
            piano.middle_c_distance_x_m -= step_m
            moved = True
        elif command == "y+":
            piano.middle_c_y_m += step_m
            moved = True
        elif command == "y-":
            piano.middle_c_y_m -= step_m
            moved = True
        elif command == "z+":
            piano.key_surface_above_support_m += step_m
            moved = True
        elif command == "z-":
            piano.key_surface_above_support_m -= step_m
            moved = True
        elif command.startswith("step "):
            step_m = float(command.split(maxsplit=1)[1]) / 1000.0
            print(f"Step set to {step_m * 1000.0:.1f} mm")
        elif command == "show":
            print_current(piano, tool)
        elif command == "save":
            save_calibration(output, piano, tool)
        elif command == "move":
            print_current(piano, tool)
            if not dry_run:
                move_to_hover(robot, ec, piano, tool, motion)
        elif command == "tap":
            print_current(piano, tool)
            if not dry_run:
                tap_middle_c(robot, ec, piano, tool, motion)
        else:
            print(f"Unknown command: {command}")

        if moved:
            print_current(piano, tool)
            if not dry_run:
                move_to_hover(robot, ec, piano, tool, motion)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Calibrate the Middle C position for piano playback.")
    parser.add_argument("--ip", default="192.168.0.160", help="Robot IP address.")
    parser.add_argument("--dry-run", action="store_true", help="Do not connect to the robot; only print target poses.")
    parser.add_argument("--output", default=str(DEFAULT_OUTPUT), help="Calibration JSON output path.")
    parser.add_argument("--middle-c-x", type=float, default=0.390, help="Initial Middle C x distance in meters.")
    parser.add_argument("--middle-c-y", type=float, default=0.0, help="Initial Middle C y offset in meters.")
    parser.add_argument("--key-z", type=float, default=0.300, help="Initial key surface height above support in meters.")
    return parser


def main() -> None:
    args = build_parser().parse_args()
    piano = build_default_piano()
    piano.middle_c_distance_x_m = args.middle_c_x
    piano.middle_c_y_m = args.middle_c_y
    piano.key_surface_above_support_m = args.key_z
    tool = build_default_tool()
    motion = build_default_motion()
    output = Path(args.output)

    robot = None
    ec = {}
    try:
        if not args.dry_run:
            player.load_sdk()
            robot = player.xCoreSDK_python.xMateRobot(args.ip)
            player.pre_op(robot, ec, motion)
        interactive_loop(robot, ec, piano, tool, motion, output, args.dry_run)
    finally:
        if robot is not None:
            robot.setNoneEventWatcher(player.xCoreSDK_python.Event.moveExecution, ec)
            robot.disconnectFromRobot(ec)


if __name__ == "__main__":
    main()
