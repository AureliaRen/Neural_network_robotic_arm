#!/usr/bin/env python3
"""
xMate SR3C Windows 真机钢琴序列弹奏脚本。

设计目标
--------
1. 尽量保持与 SDK 示例程序一致的代码风格。
2. 复用前面在标定与仿真中已经使用过的钢琴几何参数和工具末端补偿逻辑。
3. 将真实任务流程写清楚：
   回到原位 -> 移动到按键上方 -> 竖直按下 -> 保持时长 -> 抬起 -> 下一个按键。

建议使用方式
------------
先使用独立仿真检验脚本确认参数，再在 Windows 下使用本脚本将相同序列发送到真实机械臂。
"""

import time
import math
import platform
import csv
from pathlib import Path
import numpy as np
import setup_path

from log import print_log, print_separator
from piano_calibration_and_control import (
    PianoGeometry,
    ToolGeometry,
    MotionProfile,
    NoteEvent,
    build_key_centers,
    default_calibration_events,
    desired_tip_pose,
    midi_to_name,
    note_name_to_midi,
    parse_sequence_arg,
    tip_pose_to_end_pose,
)


M_PI = math.pi
EXAMPLE_DIR = Path(__file__).resolve().parent
xCoreSDK_python = None
MoveExecution = None


def load_sdk():
    """
    按需加载 SDK。

    这样在 dry_run 模式下，即使当前 Python 版本与 SDK 不匹配，
    也仍然可以先完成序列读取和目标位姿检查。
    """
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


def pre_op(robot, ec, motion):
    """
    执行与 SDK 示例程序一致的基础初始化流程。

    这里主要完成以下几件事：
    - 切换到手动模式并上电
    - 设置为非实时指令控制模式
    - 配置默认速度和过渡区
    - 注册运动事件回调，便于打印执行信息
    """
    load_sdk()
    print_separator("pre_op", length=100)

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


def wait_robot(robot, ec):
    """
    等待机械臂回到空闲状态。

    这里沿用示例程序里的轮询思路。由于钢琴弹奏任务本身是分阶段、
    点到点执行的过程，因此使用这种简单稳妥的等待方式更合适。
    """
    load_sdk()
    print_separator("wait_robot", length=80)
    running = True
    while running:
        time.sleep(0.1)
        state = robot.operationState(ec)
        if state == xCoreSDK_python.OperationState.idle:
            running = False
    print("运动执行完成")


def get_move_info(robot, ec):
    load_sdk()
    print_separator("get_move_info", length=80)
    info = robot.queryEventInfo(xCoreSDK_python.Event.moveExecution, ec)
    print_log("queryEventInfo", ec)
    return info


def print_move_info(info):
    load_sdk()
    print_separator("print_move_info", length=80)
    print(f"{MoveExecution.ID}:{info[MoveExecution.ID]}")
    print(f"{MoveExecution.ReachTarget}:{info[MoveExecution.ReachTarget]}")
    print(f"{MoveExecution.WaypointIndex}:{info[MoveExecution.WaypointIndex]}")
    print(f"{MoveExecution.Error}:{info[MoveExecution.Error]}")
    print(f"{MoveExecution.Remark}:{info[MoveExecution.Remark]}")


def get_joint_pos(robot, ec):
    load_sdk()
    print_separator("get_joint_pos", length=80)
    joint_pos = robot.jointPos(ec)
    print_log("jointPos", ec, ",".join(map(str, joint_pos)))
    return joint_pos


def get_cart_posture(robot, ec):
    """
    读取并打印当前参考坐标系下的笛卡尔位姿。
    """
    load_sdk()
    print_separator("get_cart_posture", length=80)
    cart_posture = robot.cartPosture(xCoreSDK_python.CoordinateType.endInRef, ec)
    print_log("cartPosture", ec)
    print(f"trans,{','.join(map(str, cart_posture.trans))}")
    print(f"rpy,{','.join(map(str, cart_posture.rpy))}")
    print(f"pos,{','.join(map(str, cart_posture.pos))}")
    return cart_posture


def calc_ik(robot, ec, pose6):
    """
    将笛卡尔位姿 [x, y, z, r, p, y] 转换为对应的关节角解。
    """
    load_sdk()
    print_separator("calc_ik", length=80)
    cart_pos = xCoreSDK_python.CartesianPosition(pose6)
    joint_pos = robot.model().calcIk(cart_pos, ec)
    print_log("calcIk", ec, ",".join(map(str, joint_pos)))
    if ec.get("ec", 0) != 0:
        raise RuntimeError(
            "calcIk failed for pose "
            f"({pose6[0]:.4f}, {pose6[1]:.4f}, {pose6[2]:.4f}, "
            f"{pose6[3]:.4f}, {pose6[4]:.4f}, {pose6[5]:.4f}): "
            f"{ec.get('message')}"
        )
    return joint_pos


def move_absj_to_pose(robot, ec, pose6, motion, label):
    """
    通过“逆解 + MoveAbsJ”的方式到达目标笛卡尔位姿。

    这里优先使用 MoveAbsJ，主要原因有两点：
    - 回到初始位姿、到达第一个按键上方时，用关节空间运动更稳妥
    - 可以避免在工具尚未到达安全区域前发生不必要的直线插补
    """
    load_sdk()
    print_separator(f"move_absj_to_pose_{label}", length=100)
    joint_pos = calc_ik(robot, ec, pose6)
    joint_position = xCoreSDK_python.JointPosition(joint_pos)
    absj_cmd = xCoreSDK_python.MoveAbsJCommand(joint_position, motion.move_absj_acc, motion.default_zone)
    cmd_id = xCoreSDK_python.PyString()
    robot.moveAppend([absj_cmd], cmd_id, ec)
    print("Command ID:", cmd_id.content())
    print_log("moveAppend", ec)
    robot.moveStart(ec)
    print_log("moveStart", ec)
    wait_robot(robot, ec)


def move_l_to_pose(robot, ec, pose6, speed, zone, label):
    """
    执行笛卡尔空间直线运动。

    这里使用 MoveL 的原因是：
    - 向下按键过程需要尽量保持竖直路径
    - 抬起过程需要沿原路径向上返回
    - 相邻按键上方悬停点之间的近距离运动也更便于直观分析
    """
    load_sdk()
    print_separator(f"move_l_to_pose_{label}", length=100)
    cart_pos = xCoreSDK_python.CartesianPosition(pose6)
    movel_cmd = xCoreSDK_python.MoveLCommand(cart_pos, speed, zone)
    cmd_id = xCoreSDK_python.PyString()
    robot.moveAppend([movel_cmd], cmd_id, ec)
    print("Command ID:", cmd_id.content())
    print_log("moveAppend", ec)
    robot.moveStart(ec)
    print_log("moveStart", ec)
    wait_robot(robot, ec)


def build_note_events(mode, note, sequence):
    if mode == "calibrate":
        return default_calibration_events()
    if mode == "single":
        midi = int(note) if note.lstrip("-").isdigit() else note_name_to_midi(note)
        return [NoteEvent(midi=midi, hold_s=0.30)]
    return parse_sequence_arg(sequence)


def parse_note_value(row, candidate_columns):
    """
    从一行 CSV 记录中提取音符信息。

    支持两种常见表示方式：
    - MIDI 编号，例如 76
    - 音名，例如 E5
    """
    for col in candidate_columns:
        if col not in row:
            continue
        value = str(row[col]).strip()
        if value == "":
            continue
        if value.lstrip("-").isdigit():
            return int(value)
        return note_name_to_midi(value)
    raise ValueError(f"当前 CSV 行中未找到可用的音符列，候选列为：{candidate_columns}")


def parse_hold_value(row, candidate_columns):
    """
    从一行 CSV 记录中提取按键保持时长，单位为秒。
    """
    for col in candidate_columns:
        if col not in row:
            continue
        value = str(row[col]).strip()
        if value == "":
            continue
        return float(value)
    raise ValueError(f"当前 CSV 行中未找到可用的时长列，候选列为：{candidate_columns}")


def load_note_events_from_csv(csv_path, prefer_predicted_note=True):
    """
    从之前生成的乐曲序列 CSV 中读取音符事件。

    当前兼容的常见文件包括：
    - 训练/目标序列文件：例如 note + target_hold_time_s
    - 虚拟机械臂轨迹文件：例如 source_note + tempo_s
    - 带预测结果的细节文件：例如 predicted_note + tempo_s
    """
    csv_path = Path(csv_path)
    if not csv_path.exists():
        raise FileNotFoundError(f"未找到序列文件：{csv_path}")

    note_columns = []
    if prefer_predicted_note:
        note_columns.extend(["predicted_note", "predicted_note_name"])
    note_columns.extend(["source_note", "target_note", "note", "midi", "note_name"])
    hold_columns = ["hold_s", "target_hold_time_s", "tempo_s", "duration_s", "tempo", "duration"]

    events = []
    with csv_path.open("r", encoding="utf-8-sig", newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            midi = parse_note_value(row, note_columns)
            hold_s = parse_hold_value(row, hold_columns)
            events.append(NoteEvent(midi=midi, hold_s=hold_s))

    if len(events) == 0:
        raise ValueError(f"序列文件为空或没有读出有效音符：{csv_path}")

    print_separator("csv_sequence_loaded", length=110)
    print(f"序列文件：{csv_path}")
    print(f"共读取 {len(events)} 个音符事件")
    print(f"首个音符：{midi_to_name(events[0].midi)}，时长 {events[0].hold_s:.3f}s")
    print(f"最后音符：{midi_to_name(events[-1].midi)}，时长 {events[-1].hold_s:.3f}s")
    return events


def build_home_pose(piano, tool):
    """
    构造位于中央 C 上方的安全初始位姿。

    该位姿要求白色末端朝下的圆平面保持水平，小棍保持竖直。
    同时其高度高于普通悬停高度，便于机械臂先脱离键盘区域，
    再开始后续弹奏任务。
    """
    key_centers = build_key_centers(piano)
    middle_c = key_centers[60].copy()
    middle_c[2] += tool.hover_height_m + 0.080
    return tip_pose_to_end_pose([middle_c[0], middle_c[1], middle_c[2], *tool.rpy_rad], tool)


def print_sequence_targets(events, key_centers, tool):
    print_separator("sequence_targets", length=110)
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
            f"hover_pos=({hover_pose[0]:.4f}, {hover_pose[1]:.4f}, {hover_pose[2]:.4f}), "
            f"hover_rpy=({hover_pose[3]:.4f}, {hover_pose[4]:.4f}, {hover_pose[5]:.4f}), "
            f"press_pos=({press_pose[0]:.4f}, {press_pose[1]:.4f}, {press_pose[2]:.4f}), "
            f"press_rpy=({press_pose[3]:.4f}, {press_pose[4]:.4f}, {press_pose[5]:.4f}), "
            f"hold={event.hold_s:.3f}s"
        )


def play_sequence(robot, ec, events, piano, tool, motion):
    """
    主弹奏循环。

    每个按键的运动流程固定为：
    1. 先移动到目标按键上方悬停位
    2. 再竖直向下运动到按压位
    3. 保持对应时长
    4. 再竖直抬起回到悬停位
    5. 然后继续前往下一个按键
    """
    key_centers = build_key_centers(piano)
    print_sequence_targets(events, key_centers, tool)

    home_pose = build_home_pose(piano, tool)
    move_absj_to_pose(robot, ec, home_pose, motion, "home")

    if len(events) == 0:
        raise ValueError("音符序列为空。")

    first_hover = tip_pose_to_end_pose(
        desired_tip_pose(events[0].midi, key_centers, tool, pressed=False),
        tool,
    )
    move_l_to_pose(robot, ec, first_hover, motion.travel_speed, motion.default_zone, "first_hover")

    for idx, event in enumerate(events):
        note_name = midi_to_name(event.midi)
        print_separator(f"play_note_{idx + 1}_{note_name}", length=110)

        hover_pose = tip_pose_to_end_pose(
            desired_tip_pose(event.midi, key_centers, tool, pressed=False),
            tool,
        )
        press_pose = tip_pose_to_end_pose(
            desired_tip_pose(event.midi, key_centers, tool, pressed=True),
            tool,
        )

        print(
            f"Target note={note_name}, "
            f"hover_pos=({hover_pose[0]:.4f},{hover_pose[1]:.4f},{hover_pose[2]:.4f}), "
            f"hover_rpy=({hover_pose[3]:.4f},{hover_pose[4]:.4f},{hover_pose[5]:.4f}), "
            f"press_pos=({press_pose[0]:.4f},{press_pose[1]:.4f},{press_pose[2]:.4f}), "
            f"press_rpy=({press_pose[3]:.4f},{press_pose[4]:.4f},{press_pose[5]:.4f}), "
            f"hold={event.hold_s:.3f}s"
        )

        move_l_to_pose(robot, ec, hover_pose, motion.travel_speed, motion.default_zone, f"{note_name}_hover")
        move_l_to_pose(robot, ec, press_pose, motion.press_speed, 0, f"{note_name}_press")
        time.sleep(event.hold_s)
        move_l_to_pose(robot, ec, hover_pose, motion.approach_speed, 0, f"{note_name}_lift")

    move_l_to_pose(robot, ec, home_pose, motion.travel_speed, motion.default_zone, "return_home")


def main():
    """
    参数设置区。

    这里不额外封装 argparse，而是直接把主要参数显式写在脚本里，
    便于后续在 Windows 的 example 文件夹中直接修改、现场快速联调。
    """
    ip = "192.168.0.160"

    # 真实钢琴相对于机械臂参考坐标系的几何参数。
    piano = PianoGeometry(
        support_plane_z_m=0.0,
        key_surface_above_support_m=0.46,
        middle_c_distance_x_m=0.58,
        middle_c_y_m=-0.138,
        keyboard_yaw_deg=0.0,
        white_key_spacing_m=0.023,
        black_key_forward_offset_m=-0.045,
        black_key_height_offset_m=0.012,
    )

    # 工具末端补偿以白色末端朝下圆平面的中心为参考。
    # x = 0 表示小棍与该圆面中心轴对齐。
    tool = ToolGeometry(
        rpy_rad=(-M_PI, 0.0, -M_PI),
        tip_offset_local_m=(0.0, 0.0, -0.287),
        hover_height_m=0.020,
        press_depth_m=0.017,
    )

    # 针对钢琴弹奏过程设置的较保守运动参数。
    motion = MotionProfile(
        default_speed=2480,
        default_zone=0,
        travel_speed=2940,
        approach_speed=2980,
        press_speed=2980,
        home_speed=2920,
        move_absj_acc=2800,
        move_line_acc=2900,
    )

    # 选择以下三种运行模式之一：
    # - calibrate：访问中央 C 附近若干参考键位，用于现场检查
    # - single：只弹奏一个音符
    # - sequence：弹奏一段音符序列
    mode = "sequence"
    single_note = "60"
    # 文本序列格式统一为“音高:时值”，其中音高使用 MIDI 数字编号。
    # 例如 76:0.25 表示 MIDI 76 的音符保持 0.25 秒。
    sequence_text = "76:0.25,76:0.25,77:0.25,79:0.25,79:0.25,77:0.25,76:0.25,74:0.25,72:0.40"
    # 小星星备用序列。需要使用时，把下面这一段取消注释，并将 sequence_source 改为 "text"。
    # sequence_text = (
    #     "C4:0.30,C4:0.30,G4:0.30,G4:0.30,A4:0.30,A4:0.30,G4:0.60,"
    #     "F4:0.30,F4:0.30,E4:0.30,E4:0.30,D4:0.30,D4:0.30,C4:0.60,"
    #     "G4:0.30,G4:0.30,F4:0.30,F4:0.30,E4:0.30,E4:0.30,D4:0.60,"
    #     "G4:0.30,G4:0.30,F4:0.30,F4:0.30,E4:0.30,E4:0.30,D4:0.60,"
    #     "C4:0.30,C4:0.30,G4:0.30,G4:0.30,A4:0.30,A4:0.30,G4:0.60,"
    #     "F4:0.30,F4:0.30,E4:0.30,E4:0.30,D4:0.30,D4:0.30,C4:0.60"
    # )

    # 序列输入方式：
    # - csv：读取之前生成的乐曲序列文件
    # - text：直接使用上面的 sequence_text
    sequence_source = "csv"

    # 这里直接填写序列文件的完整路径，后续换电脑时改这一行最方便。
    # 训练序列示例：
    # sequence_csv_path = r"C:\Users\SZH\Desktop\test\job\music-motor-control\result_output\sequence_learning\fur_elise_training_data.csv"
    # 预测结果示例：
    # sequence_csv_path = r"C:\Users\SZH\Desktop\test\job\music-motor-control\result_output\sequence_learning\fur_elise_final_noise_1p5_details.csv"
    sequence_csv_path = r"E:\arm_sdk\xcoresdk_python-v0.4.1.C118_0\example\fur_elise_training_data.csv"

    # 读取带预测结果的细节 CSV 时，优先使用 predicted_note；
    # 如果该列不存在，则自动回退到 note/source_note 等列。
    prefer_predicted_note_from_csv = True

    # 若只想先测试前面一小段序列，可在这里限制音符数量。
    # 例如 24 表示只弹奏前 24 个音符；设为 None 表示使用完整序列。
    max_notes_to_play = 24


    # 若为 True，则只打印目标位姿，不驱动真实机械臂运动。
    dry_run = False

    if mode == "sequence" and sequence_source == "csv":
        events = load_note_events_from_csv(Path(sequence_csv_path), prefer_predicted_note=prefer_predicted_note_from_csv)
    else:
        events = build_note_events(mode, single_note, sequence_text)

    if max_notes_to_play is not None:
        events = events[:max_notes_to_play]
        print_separator("sequence_clip", length=110)
        print(f"当前仅使用前 {len(events)} 个音符进行测试")

    key_centers = build_key_centers(piano)
    print_separator("reference_points", length=110)
    for midi in [48, 60, 72]:
        pos = key_centers[midi]
        print(f"{midi_to_name(midi)}: x={pos[0]:.4f}, y={pos[1]:.4f}, z={pos[2]:.4f}")

    if dry_run:
        print_sequence_targets(events, key_centers, tool)
        return

    robot = None
    ec = {}
    try:
        load_sdk()
        robot = xCoreSDK_python.xMateRobot(ip)
        pre_op(robot, ec, motion)
        get_joint_pos(robot, ec)
        get_cart_posture(robot, ec)
        play_sequence(robot, ec, events, piano, tool, motion)
        print_move_info(get_move_info(robot, ec))
        get_joint_pos(robot, ec)
        get_cart_posture(robot, ec)
    except Exception as e:
        print(f"程序运行出现异常：{e}")
        raise
    finally:
        if robot is not None:
            robot.setNoneEventWatcher(xCoreSDK_python.Event.moveExecution, ec)
            robot.disconnectFromRobot(ec)


if __name__ == "__main__":
    main()
