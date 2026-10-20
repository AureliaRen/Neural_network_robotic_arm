'''运动相关的示例'''
import time
import math
import sys
import setup_path
import platform
import numpy as np
import math
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

M_PI = math.pi
   
def move_absJ(robot,ec):
    '''moveAbsJ运动'''
    print_separator("move_absJ",length=80)
    joint_list1 = [21.95,0.07,-59.97,-6.59,-23.28,1.59]
    joint_list1 = np.array(joint_list1) * np.pi / 180
    joint_list1 = list(joint_list1)
    joint_list2 = [31, 0.02, -123.54, -1.33, -59.27, 3.38]
    joint_list2 = np.array(joint_list2) * np.pi / 180
    joint_list2 = list(joint_list2)
    joint_pos1 = xCoreSDK_python.JointPosition(joint_list1)
    absjcmd1 = xCoreSDK_python.MoveAbsJCommand(joint_pos1,1000,10)
    joint_pos2 = xCoreSDK_python.JointPosition(joint_list2)
    absjcmd2 = xCoreSDK_python.MoveAbsJCommand(joint_pos2, 1000, 10)
    cmdID = xCoreSDK_python.PyString()
    robot.moveAppend([absjcmd1,absjcmd2], cmdID, ec)# [absjcmd]指令列表，可以添加多条指令，须为同类型指令
    print("Command ID:", cmdID.content())
    print_log("moveAppend",ec)
    robot.moveStart(ec)
    print_log("moveStart",ec)
    wait_robot(robot,ec)
        
def moveL(robot,ec):
    '''moveL运动'''
    print_separator("moveL",length=80)
    cart_pos = xCoreSDK_python.CartesianPosition([-0.036,0.240,0.731,-52.593*3.14/180,-27*3.14/180,-174.55*3.14/180])  #s4点位
    movelcmd = xCoreSDK_python.MoveLCommand(cart_pos,50,0)
    cmdID = xCoreSDK_python.PyString()
    robot.moveAppend([movelcmd], cmdID, ec)# [movelcmd]指令列表，可以添加多条指令，须为同类型指令
    print("Command ID:", cmdID.content())
    print_log("moveAppend",ec)
    robot.moveStart(ec)
    print_log("moveStart",ec)
    wait_robot(robot,ec)
    
def moveJ(robot,ec):
    '''moveJ运动'''
    print_separator("moveJ",length=80)
    cart_pos = xCoreSDK_python.CartesianPosition([0.614711,0.136,0.416211,-M_PI,0,-M_PI])  #s4点位
    movejcmd = xCoreSDK_python.MoveJCommand(cart_pos,1000,10)
    cmdID = xCoreSDK_python.PyString()
    robot.moveAppend([movejcmd], cmdID, ec)# [movejcmd]指令列表，可以添加多条指令，须为同类型指令
    print("Command ID:", cmdID.content())
    print_log("moveAppend",ec)
    robot.moveStart(ec)
    print_log("moveStart",ec)
    wait_robot(robot,ec)

def moveC(robot,ec):
    '''moveC运动'''
    print_separator("moveC",length=80)
    target = xCoreSDK_python.CartesianPosition([0.214711,0.236,0.616211,-M_PI,0,-M_PI])  #s4点位
    aux = xCoreSDK_python.CartesianPosition([0.414711,0.236,0.416211,-M_PI,0,-M_PI])  #s4点位
    moveccmd = xCoreSDK_python.MoveCCommand(target,aux,1000,10)
    cmdID = xCoreSDK_python.PyString()
    robot.moveAppend([moveccmd], cmdID,ec)
    print("Command ID:", cmdID.content())
    print_log("moveAppend",ec)
    robot.moveStart(ec)
    print_log("moveStart",ec)
    wait_robot(robot,ec)

    
def moveSP(robot,ec):
    '''moveSP运动'''
    print_separator("moveSP",length=80)
    target = xCoreSDK_python.CartesianPosition([0.214711,0.136,0.416211,-M_PI,0,-M_PI])  #s4点位
    r0 = 0.1
    rstep = 0.05
    angle = 1
    dir = True
    movesp_cmd = xCoreSDK_python.MoveSPCommand(target,r0,rstep,angle,dir,1000)
    cmdID = xCoreSDK_python.PyString()
    robot.moveAppend([movesp_cmd], cmdID,ec)
    print("Command ID:", cmdID.content())
    print_log("moveAppend",ec)
    robot.moveStart(ec)
    print_log("moveStart",ec)
    wait_robot(robot,ec)
        
def wait_robot(robot,ec):
    '''等待运动结束 - 通过查询机械臂是否处于运动中的方式'''
    print_separator("wait_robot",length=80)
    running = True
    while (running):
        time.sleep(0.1)
        st = robot.operationState(ec)
        if(st == xCoreSDK_python.OperationState.idle or st == xCoreSDK_python.OperationState.unknown):
            running = False
    print("move finished")   
    
def pause_and_continue(robot,ec):
    '''暂停和继续'''
    print_separator("pause and continue",length=80)
    joint_pos = xCoreSDK_python.JointPosition([0,0,0,0,0,0])
    absjcmd = xCoreSDK_python.MoveAbsJCommand(joint_pos,1000,10)
    cmdID = xCoreSDK_python.PyString()
    robot.moveAppend([absjcmd], cmdID, ec)# [absjcmd]指令列表，可以添加多条指令，须为同类型指令
    print_log("moveAppend",ec)
    robot.moveStart(ec)
    print("start")
    time.sleep(2) # 等待2秒后暂停
    robot.stop(ec)
    print("pause")
    time.sleep(2) # 等待2秒后继续
    robot.moveStart(ec)
    print("continue")
    time.sleep(10) # 等待2秒后结束
    robot.stop(ec)
    print("stop")
            
def query_controller_log(robot,ec):
    '''查询控制器日志'''
    print_separator("query controller log",length=80)
    # 查询最近5条错误级别控制器日志
    controller_logs = robot.queryControllerLog(5, {xCoreSDK_python.LogInfoLevel.error}, ec)
    print_log("queryControllerLog",ec)
    for log in controller_logs:
        print(log.content)
          
def get_move_info(robot,ec):
    '''获取运动信息'''
    print_separator("get move info",length=80)
    info = robot.queryEventInfo(xCoreSDK_python.Event.moveExecution,ec)
    print_log("queryEventInfo",ec)
    return info

def print_move_info(info:dict):
    '''打印运动执行信息'''
    print_separator("print move info",length=80)
    print(f"{MoveExecution.ID}:{info[MoveExecution.ID]}")
    print(f"{MoveExecution.ReachTarget}:{info[MoveExecution.ReachTarget]}")
    print(f"{MoveExecution.WaypointIndex}:{info[MoveExecution.WaypointIndex]}")
    print(f"{MoveExecution.Error}:{info[MoveExecution.Error]}")
    print(f"{MoveExecution.Remark}:{info[MoveExecution.Remark]}")

def get_joint_pos(robot,ec):
    '''获取关节位置'''
    print_separator("get_joint_pos",length=80)
    joint_pos = robot.jointPos(ec)
    print_log("jointPos",ec,','.join(map(str,joint_pos)))

def get_cart_posture(robot,ec):
    '''获取当前笛卡尔坐标信息'''
    print_separator("get_cart_posture",length=80)
    cart_posture = robot.cartPosture(xCoreSDK_python.CoordinateType.endInRef, ec)
    print_log("cartPosture",ec)
    print(f"elbow,{cart_posture.elbow}")
    print(f"hasElbow,{cart_posture.hasElbow}")
    print(f"confData,f{','.join(map(str,cart_posture.confData))}")
    print(f"external size,{len(cart_posture.external)}")
    print(f"trans,{','.join(map(str,cart_posture.trans))}")
    print(f"rpy,{','.join(map(str,cart_posture.rpy))}")
    print(f"pos,{','.join(map(str,cart_posture.pos))}")

def get_posture(robot,ec):
    '''获取当前位姿'''
    print_separator("get_posture",length=80)
    pos = robot.posture(xCoreSDK_python.CoordinateType.endInRef, ec)
    print_log("posture",ec,', '.join(map(str, pos)))

def move0(robot, ec, center, radius, num_points=36):
    '''使机械臂实现画圆圈的运动'''
    print_separator("move0 - draw circle", length=80)

    # 计算圆周上的点
    points = []
    for i in range(num_points):
        angle = 2 * math.pi * i / num_points
        x = center[0] + radius * math.cos(angle)
        y = center[1] + radius * math.sin(angle)
        points.append((x, y))

    # 依次移动机械臂到这些点，假设Z维度保持不变
    for point in points:
        point=points[0]
        cart_pos = xCoreSDK_python.CartesianPosition([point[0], point[1], center[2], 3.14, -0.06, -1.51])
        movelcmd = xCoreSDK_python.MoveJCommand(cart_pos, 500, 0)
        cmdID = xCoreSDK_python.PyString()
        robot.moveAppend([movelcmd], cmdID, ec)
        # 将单条指令加入队列
        # 等待1秒后执行指令
        #time.sleep(1)
       # robot.moveStart(ec)
    # 获取关节位置和笛卡尔位姿
    get_joint_pos(robot, ec)
    get_cart_posture(robot, ec)

def move_arc(robot, ec, center, radius,angle, num_points=36):
    '''使机械臂实现画圆弧的运动'''
    print_separator("draw arc", length=80)

    # 计算圆周上的点
    points = []
    for i in range(num_points):
        arc =  (angle/180)*math.pi*i / num_points
        x = center[0] + radius * math.cos(arc)
        y = center[1] + radius * math.sin(arc)
        points.append((x, y))

    # 依次移动机械臂到这些点，假设Z维度保持不变
    for point in points:
        point=points[0]
        cart_pos = xCoreSDK_python.CartesianPosition([point[0], point[1], center[2], 3.14, -0.06, -1.51])
        movelcmd = xCoreSDK_python.MoveJCommand(cart_pos, 500, 0)
        cmdID = xCoreSDK_python.PyString()
        robot.moveAppend([movelcmd], cmdID, ec)
        # 将单条指令加入队列
        # 等待1秒后执行指令
        time.sleep(1)
        robot.moveStart(ec)
    # 获取关节位置和笛卡尔位姿
    get_joint_pos(robot, ec)
    get_cart_posture(robot, ec)


    