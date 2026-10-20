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

def main():
    try:
        ip = "192.168.0.160"
        robot = xCoreSDK_python.xMateRobot(ip)
        ec = {}
        init.pre_op(robot, ec)
        example.get_joint_pos(robot, ec)
        print("joint_pos")
    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    main()
