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

def circular_motion(robot,ec):
    print_separator("circular_motion",length=110)
    cart_posture = robot.cartPosture(xCoreSDK_python.CoordinateType.endInRef, ec)
    r=math.sqrt(cart_posture.trans[0]*cart_posture.trans[0]+cart_posture.trans[1]*cart_posture.trans[1])
    center=[0.0,0.0,cart_posture.trans[2]]
    example.move_arc(robot,ec,center,r,30)
    example.wait_robot(robot,ec)

def main():
    try:
        ip = "192.168.0.160"
        robot = xCoreSDK_python.xMateRobot(ip)
        ec = {}
        init.pre_op(robot, ec)
        init.pos_init(robot,ec)
        example.get_joint_pos(robot, ec)
        circular_motion(robot,ec)
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