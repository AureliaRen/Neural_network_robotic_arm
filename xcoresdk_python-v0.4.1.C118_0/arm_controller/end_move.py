'''运动相关的示例'''
import init
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
from operator  import add
M_PI = math.pi
step = 8.0
def end_move(robot,ec):
    print_separator("end_move",length=110)
    init.pre_op(robot,ec)
    op=int(input("请输入操作数:0/向左,1/向右\n"))
    # op==0?move_left:move_right
    # move:机械臂末端从上往下看绕z轴逆时针旋转为正，顺时针旋转为负
    '''
    1.获取机械臂末端位姿
    2.计算机械臂各关节角度与末端坐标的雅可比矩阵
    3.逆运动学反解机械臂各关节旋转速度
    '''
    cart_posture = robot.cartPosture(xCoreSDK_python.CoordinateType.endInRef,ec)
    left_vector = [0.0,0.0,1.0*M_PI/180*step]
    right_vector = [0.0,0.0,-1.0*M_PI/180*step]
    if op == 0 :
        cart_posture.rpy=list(map(add, cart_posture.rpy,left_vector))
    elif op == 1 :
        cart_posture.rpy=list(map(add,cart_posture.rpy,right_vector))
    else:
        print('非法操作数，已取消本次运动')
        return
    robot_model1 = robot.model()
    joint_pos = robot.jointPos(ec)
    target_pos = robot_model1.calcIk(cart_posture,ec)
    if ec['ec'] != 0:
        print_log('calcIk', ec)
        print('逆解失败，已取消本次运动')
        return
    absjcmd1 = xCoreSDK_python.MoveAbsJCommand(joint_pos,1000,10)
    absjcmd2 = xCoreSDK_python.MoveAbsJCommand(target_pos,1000,10)
    cmdID= xCoreSDK_python.PyString()
    robot.moveAppend([absjcmd1,absjcmd2],cmdID,ec)
    robot.moveStart(ec)
    example.wait_robot(robot,ec)

def main():
    try:
        ip = "192.168.0.160"
        robot = xCoreSDK_python.xMateRobot(ip)
        ec = {}
        print_separator("move_op", length=110)
        init.pre_op(robot, ec)
        example.get_joint_pos(robot, ec)
        end_move(robot,ec)
        # example.print_move_info(example.get_move_info(robot, ec))
        robot.setNoneEventWatcher(xCoreSDK_python.Event.moveExecution, ec)  # 结束时需要关闭监视器
        print("joint_pos")
        example.get_joint_pos(robot, ec)
        print("cart_posture")
        example.get_cart_posture(robot, ec)
    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    main()
