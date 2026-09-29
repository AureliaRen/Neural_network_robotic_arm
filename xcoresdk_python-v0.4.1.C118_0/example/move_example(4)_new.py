'''运动相关的示例'''
import time
import math
import pandas as pd
import sys
import setup_path
import platform
import numpy as np
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

def move_op(robot,ec):
    print_separator("move_op",length=110)
    pre_op(robot,ec)
    #calcFk(robot,ec)
    #calcIk(robot,ec)
    move_absJ(robot,ec)
    #moveL(robot,ec)
    #moveJ(robot,ec)
    #moveC(robot,ec)
    #moveCF(robot,ec)
    #moveSP(robot,ec)
    pause_and_continue(robot,ec)
    print_move_info(get_move_info(robot,ec))
    query_controller_log(robot,ec)
    robot.setNoneEventWatcher(xCoreSDK_python.Event.moveExecution, ec) # 结束时需要关闭监视器
    
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

def calcFk(robot,ec):
    '''计算正解，关节角度->笛卡尔坐标''' 
    print_separator("calcFk",length=80)
    start_angle = [0, 0.557737,-1.5184888, 0,-1.3036738, 0] # 单位弧度
    robot_model = robot.model()
    cart_pose = robot_model.calcFk(start_angle, ec)   
    print_log("calcFk",ec)
    print(f"elbow,{cart_pose.elbow}")
    print(f"hasElbow,{cart_pose.hasElbow}")
    print(f"confData,f{','.join(map(str,cart_pose.confData))}")
    print(f"external size,{len(cart_pose.external)}")
    print(f"trans,{','.join(map(str,cart_pose.trans))}")
    print(f"rpy,{','.join(map(str,cart_pose.rpy))}")
    print(f"pos,{','.join(map(str,cart_pose.pos))}")

def calcIk(robot,ec):
    '''计算逆解，笛卡尔坐标 -> 关节角度'''
    print_separator("calcIk",length=80)
    cart_pos = xCoreSDK_python.CartesianPosition([0.60,0.13,0.41,-3.14,-0.23,-3.14])  #s4点位
    robot_model = robot.model()
    joint_pos = robot_model.calcIk(cart_pos,ec)   
    print_log("calcIk",ec,','.join(map(str,joint_pos)))
     
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
    
def moveCF(robot,ec):
    '''moveCF运动'''
    print_separator("moveCF",length=80)
    target = xCoreSDK_python.CartesianPosition([0.614711,0.136,0.416211,-1.57,0,-1.57])  #s4点位
    aux = xCoreSDK_python.CartesianPosition([0.614711,0.236,0.416211,-1.57,0,-1.57])  #s4点位
    movecf_cmd = xCoreSDK_python.MoveCFCommand(target,aux,2,1000,10)
    cmdID = xCoreSDK_python.PyString()
    robot.moveAppend([movecf_cmd], cmdID,ec)
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

def get_joint_pos(robot,ec):
    '''获取关节位置'''
    print_separator("get_joint_pos",length=80)
    joint_pos = robot.jointPos(ec)
    print_log("jointPos",ec,','.join(map(str,joint_pos)))

def move_circle(robot,ec):
    '''move_write_letter运动'''
    print_separator("move_write_letter",length=80)

# 初始化参数
    cart_pos=[]
    moveccmd = []
    moveccmd2 =[]
    h1=0.137   # 高度
    h2=0.167   # 抬笔高度
    x_f=0.21   # 横向'初'位置
    dx = 0.04  # 字间距

# 数据导入
    # 导入data1数据 (o)
    file_path = 'new_processed_data1.xlsx'
    df = pd.read_excel(file_path, sheet_name=0)
    x = df.iloc[:, 0]  # 第一列数据
    y = df.iloc[:, 1]  # 第二列数据
    print(x)
    print(y)
    # 导入data2数据 (j)
    file_path = 'new_processed_data2.xlsx'
    df = pd.read_excel(file_path, sheet_name=0)
    x2 = df.iloc[:, 0]  # 第一列数据
    y2 = df.iloc[:, 1]  # 第二列数据
    print(x2)
    print(y2)
    # 导入data3数据 (l)
    file_path = 'new_processed_data3.xlsx'
    df = pd.read_excel(file_path, sheet_name=0)
    x3 = df.iloc[:, 0]  # 第一列数据
    y3 = df.iloc[:, 1]  # 第二列数据
    print(x3)
    print(y3)

# 将数据导入位置列表(包括抬笔数据）
    for i in range(len(x)):
        cart_pos.append(xCoreSDK_python.CartesianPosition([x.iloc[i]+x_f, y.iloc[i]-0.65, h1, M_PI/2, M_PI / 2, -M_PI]))  # i=len(x)-1
    for i in range(len(x2)):
        if i < len(x2)-1:
            cart_pos.append(xCoreSDK_python.CartesianPosition([x2.iloc[i]+x_f, y2.iloc[i]-0.65, h1, M_PI/2, M_PI / 2, -M_PI]))
        else:
            cart_pos.append(xCoreSDK_python.CartesianPosition([x2.iloc[i] + x_f, y2.iloc[i] - 0.65, h2, M_PI / 2, M_PI / 2, -M_PI]))   # 抬笔 i=len(x)+len(x2)-1
    for i in range(len(x)):
        if i < len(x) - 1:
            cart_pos.append(xCoreSDK_python.CartesianPosition([x.iloc[i]+x_f+dx, y.iloc[i]-0.65, h1, M_PI/2, M_PI / 2, -M_PI]))
        else:
            cart_pos.append(xCoreSDK_python.CartesianPosition([x.iloc[i] + x_f +dx, y.iloc[i] - 0.65, h2, M_PI / 2, M_PI / 2, -M_PI]))  # 抬笔 i=2*len(x)+len(x2)-1
    for i in range(len(x)):
        if i < len(x) - 1:
            cart_pos.append(xCoreSDK_python.CartesianPosition([x.iloc[i]+x_f+2*dx, y.iloc[i]-0.65, h1, M_PI/2, M_PI / 2, -M_PI]))
        else:
            cart_pos.append(xCoreSDK_python.CartesianPosition([x.iloc[i] + x_f + 2*dx, y.iloc[i] - 0.65, h2, M_PI / 2, M_PI / 2, -M_PI]))  # 抬笔 i=3*len(x)+len(x2)-1
    for i in range(len(x3)):
        cart_pos.append(xCoreSDK_python.CartesianPosition([x3.iloc[i]+x_f+3*dx, y3.iloc[i]-0.65, h1, M_PI/2, M_PI / 2, -M_PI]))        # i=3*len(x)+len(x2)+len(x3)-1
    for i in range(len(x)):
        if i<len(x)-1:
            cart_pos.append(xCoreSDK_python.CartesianPosition([x.iloc[i]+x_f+3*dx, y.iloc[i]-0.65, h1, M_PI/2, M_PI / 2, -M_PI]))      # i=4*len(x)+len(x2)+len(x3)-1
        else:
            cart_pos.append(xCoreSDK_python.CartesianPosition([x.iloc[i] + x_f + 3 * dx, y.iloc[i] - 0.65, h2, M_PI / 2, M_PI / 2, -M_PI])) # 抬笔 i=3*len(x)+len(x2)-1
# 定义指令列表
    "将cart_pos[0]转化为joint_pos"
    robot_model = robot.model()
    joint_pos = robot_model.calcIk(cart_pos[0], ec)   #i=0

    "定义moveccmd指令列表"
    for j in range(0,-2+len(x),2):
         moveccmd.append(xCoreSDK_python.MoveCCommand(cart_pos[j+2] ,cart_pos[j+1], 1000, 10))     # i=0 ----i=len(x)-1
    for j in range(len(x),-2+len(x)+len(x2),2):
         moveccmd.append(xCoreSDK_python.MoveCCommand(cart_pos[j+2] ,cart_pos[j+1], 1000, 10))     # i=len(x) ----i=len(x)+len(x2)-1
    for j in range(len(x)+len(x2), len(x)+len(x2)+len(x)-2, 2):
        moveccmd.append(xCoreSDK_python.MoveCCommand(cart_pos[j + 2], cart_pos[j + 1], 1000, 10))  # i=len(x)+len(x2) ----i=2*len(x)+len(x2)-1
    for j in range(2*len(x)+len(x2), 3*len(x)+len(x2)-2, 2):
        moveccmd.append(xCoreSDK_python.MoveCCommand(cart_pos[j + 2], cart_pos[j + 1], 1000, 10))  # i=2*len(x)+len(x2) ----i=3*len(x)+len(x2)-1
    for j in range(3*len(x)+len(x2), 3*len(x)+len(x2)+len(x3)-2, 2):
        moveccmd.append(xCoreSDK_python.MoveCCommand(cart_pos[j + 2], cart_pos[j + 1], 1000, 10))  # i=3*len(x)+len(x2) ----i=3*len(x)+len(x2)+len(x3)-1
    for j in range(3*len(x)+len(x2)+len(x3), 4*len(x)+len(x2)+len(x3)-2, 2):
        moveccmd2.append(xCoreSDK_python.MoveCCommand(cart_pos[j + 2], cart_pos[j + 1], 1000, 10)) # i=4*len(x)+len(x2) ----i=4*len(x)+len(x2)+len(x3)-1

# 根据指令列表运动
    "先移动到起始点"
    absjcmd = xCoreSDK_python.MoveAbsJCommand(joint_pos, 1000, 10)
    cmdID = xCoreSDK_python.PyString()
    robot.moveAppend([absjcmd], cmdID, ec)  # [absjcmd]指令列表，可以添加多条指令，须为同类型指令
    "moveC运动"
    robot.moveAppend(moveccmd, cmdID, ec)
    robot.moveAppend(moveccmd2, cmdID, ec)

    print("Command ID:", cmdID.content())
    print_log("moveAppend", ec)
    robot.moveStart(ec)
    print_log("moveStart", ec)
    wait_robot(robot, ec)



def main():
    try:
        ip = "192.168.0.160"
        robot = xCoreSDK_python.xMateRobot(ip)
        ec = {}
        print_separator("move_op", length=110)
        pre_op(robot, ec)
        get_joint_pos(robot, ec)
        move_circle(robot,ec)
        #moveC(robot, ec)
        print_move_info(get_move_info(robot, ec))
        # query_controller_log(robot, ec)
        robot.setNoneEventWatcher(xCoreSDK_python.Event.moveExecution, ec)  # 结束时需要关闭监视器
        print("joint_pos")
        get_joint_pos(robot, ec)
        print("cart_posture")
        get_cart_posture(robot, ec)
        #move_op(robot,ec)
    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    main()
    