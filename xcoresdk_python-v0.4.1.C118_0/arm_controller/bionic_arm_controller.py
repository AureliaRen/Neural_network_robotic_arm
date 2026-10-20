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
step = 0.1
def end_move(robot,ec):
    init.pos_init(robot,ec)
    print_separator("simple_move",length=110)
    op=int(input("请输入操作数:-1/向左,1/向右,0/不动,2/退出\n"))
    while op != 2:
        left_vector = [1.0*step,0.0,0.0,0.0,0.0,0.0]
        right_vector = [-1.0*step,0.0,0.0,0.0,0.0,0.0]
        joint_pos = robot.jointPos(ec)
        if op == -1 :
            target_pos=list(map(add, joint_pos,left_vector))
        elif op == 1 :
            target_pos=list(map(add, joint_pos,right_vector))
        elif op == 0 :
            op=int(input("请输入操作数:-1/向左,1/向右,0/不动,2/退出\n"))
            continue
        else:
            print('非法操作数，已取消本次运动')
            return
        absjcmd1 = xCoreSDK_python.MoveAbsJCommand(joint_pos,1000,10)
        absjcmd2 = xCoreSDK_python.MoveAbsJCommand(target_pos,1000,10)
        cmdID= xCoreSDK_python.PyString()
        robot.moveAppend([absjcmd1,absjcmd2],cmdID,ec)
        robot.moveStart(ec)
        example.wait_robot(robot,ec)
        op=int(input("请输入操作数:-1/向左,1/向右,0/不动,2/退出\n"))
    return

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
