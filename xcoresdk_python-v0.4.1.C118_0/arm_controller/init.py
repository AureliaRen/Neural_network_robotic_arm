import time
import math
import sys
import setup_path
import platform
import numpy as np
import math
import move_example as example
# 根据操作系统导入相应的模块
if platform.system() == 'Windows':
    from Release.windows import xCoreSDK_python
    from Release.windows.xCoreSDK_python.EventInfoKey import MoveExecution
elif platform.system() == 'Linux':
    from Release.linux import xCoreSDK_python
    from Release.linux.xCoreSDK_python.EventInfoKey import MoveExecution
else:
    raise ImportError("Unsupported operating system")
from log import print_log, print_separator
def pre_op(robot,ec):
    '''预操作'''
    print_separator("pre_op",length=80)
    # 切换到自动模式并上电
    robot.setOperateMode(xCoreSDK_python.OperateMode.manual, ec)
    print_log("setOperateMode",ec)
    robot.setPowerState(True, ec)
    print_log("setPowerState",ec)
    # 设置默认运动速度和转弯区
    robot.setMotionControlMode(xCoreSDK_python.MotionControlMode.NrtCommandMode, ec)
    print_log("setMotionControlMode",ec)
    set_default_zone(robot,ec)
    set_default_speed(robot,ec)
    # 可选：设置运动指令执行完成和错误信息回调
    robot.setEventWatcher(xCoreSDK_python.Event.moveExecution, print_move_info, ec)

def set_default_zone(robot,ec):
    '''设置默认转弯区'''
    print_separator("set_default_zone",length=80)
    robot.setDefaultZone(50, ec); # 可选：设置默认转弯区
    print_log("setDefaultZone",ec)

def set_default_speed(robot,ec):
    '''设置默认速度'''
    print_separator("set_default_speed",length=80)
    robot.setDefaultSpeed(200, ec); # 可选：设置默认速度
    print_log("setDefaultSpeed",ec)

def print_move_info(info:dict):
    '''打印运动执行信息'''
    print_separator("print move info",length=80)
    print(f"{MoveExecution.ID}:{info[MoveExecution.ID]}")
    print(f"{MoveExecution.ReachTarget}:{info[MoveExecution.ReachTarget]}")
    print(f"{MoveExecution.WaypointIndex}:{info[MoveExecution.WaypointIndex]}")
    print(f"{MoveExecution.Error}:{info[MoveExecution.Error]}")
    print(f"{MoveExecution.Remark}:{info[MoveExecution.Remark]}")

#初始关节位姿-0.4709558705936208,0.8340217591059514,-0.5395489799990014,0.10277599373484364,-0.2651989129306356,-0.14901233184219984

def pos_init(robot,ec):
    robot_model1 = robot.model()
    joint_pos = robot.jointPos(ec)
    target_pos = [-0.4709558705936208,0.8340217591059514,-0.5395489799990014,0.10277599373484364,-0.2651989129306356,-0.14901233184219984]
    absjcmd1 = xCoreSDK_python.MoveAbsJCommand(joint_pos,1000,10)
    absjcmd2 = xCoreSDK_python.MoveAbsJCommand(target_pos,1000,10)
    cmdID= xCoreSDK_python.PyString()
    robot.moveAppend([absjcmd1,absjcmd2],cmdID,ec)
    robot.moveStart(ec)
    example.wait_robot(robot,ec)