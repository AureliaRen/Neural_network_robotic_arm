"""读取机器人状态数据示例"""
import setup_path
import numpy as np
import platform
# 根据操作系统导入相应的模块
if platform.system() == 'Windows':
    from Release.windows import xCoreSDK_python
    from Release.windows.xCoreSDK_python import RtSupportedFields
elif platform.system() == 'Linux':
    from Release.linux import xCoreSDK_python
    from Release.linux.xCoreSDK_python import RtSupportedFields
else:
    raise ImportError("Unsupported operating system")

from log import print_log, print_separator
from move_example import wait_robot
import threading
import time
import math
from datetime import timedelta
from data_collect_single_arm import single_arm_movement

tcpPose_m = RtSupportedFields.tcpPose_m
jointPos_m = RtSupportedFields.jointPos_m
M_PI = math.pi

def read_robot_state_op(robot,ec):
    print_separator("read_robot_state_op",length=110)
    robot.setMotionControlMode(xCoreSDK_python.MotionControlMode.NrtCommandMode, ec)
    print_log("setMotionControlMode", ec)
    
    # 设置数据发送间隔为1s, 接收机器人末端位姿、关节力矩和关节角度
    robot.startReceiveRobotState(timedelta(seconds=1), [tcpPose_m, jointPos_m])
    
    running = True

    # 接收状态数据的队列不会自动覆盖旧数据，可以通过循环读取的方法清除旧数据
    #while robot.updateRobotState(timedelta(milliseconds=0)):
        #pass

    # 打印末端位姿和关节角度到控制台
    def read_state():      
        while running:
            # 周期性获取当前状态数据，参数timeout最好和设置的数据发送间隔保持一致
            # 或者按照发送频率读取
            tcpPose = xCoreSDK_python.PyTypeVectorDouble()
            arr6= xCoreSDK_python.PyTypeVectorDouble()
            robot.updateRobotState(timedelta(seconds=1))
            robot.getStateData(tcpPose_m, tcpPose,16)
            robot.getStateData(jointPos_m, arr6,6)
            print("TCP pose:", tcpPose.content(), "\nJoint:", arr6.content())

    read_thread = threading.Thread(target=read_state) #创建一个读取状态数据的线程
    read_thread.start()

    # 开始一个运动线程
    def move_robot():
        robot.setOperateMode(xCoreSDK_python.OperateMode.automatic, ec)
        robot.setPowerState(True, ec)
        robot.moveReset(ec)
        #p1 = xCoreSDK_python.MoveAbsJCommand([0,0,0,0,0,0],100)
        #p2 = xCoreSDK_python.MoveAbsJCommand([0, M_PI/6, M_PI/3, 0, M_PI/2, 0],100) # sr4
        #id = xCoreSDK_python.PyString()
        #robot.moveAppend([p2], id, ec)
        #robot.moveStart(ec)
        init_list_r = [10, 45, -45, 0, 0, 0]
        robot_r, ec_r = single_arm_movement(robot, ec, init_list_r, move_type="joint_space")  # 回到初始位姿
        list_r = [0.335, 0.15, 0.50, np.pi / 2, 0, 0]
        robot_r, ec_r = single_arm_movement(robot, ec_r, list_r, move_type="absj")  # 到达抓取位置
        list_r = [0.402, 0.10, 0.054, np.pi / 2, np.pi / 2, 0]
        robot_r, ec_r = single_arm_movement(robot_r, ec_r, list_r, move_type="cartesian")
        wait_robot(robot_r,ec)

    move_thread = threading.Thread(target=move_robot)
    move_thread.start()

    # 等待运动结束
    move_thread.join()
    running = False
    read_thread.join()

    # 控制器停止发送
    robot.stopReceiveRobotState()
        
if __name__ == '__main__':
    try:
        ip = "192.168.0.160"
        local_ip = "192.168.2.100"
        robot = xCoreSDK_python.xMateRobot(ip, local_ip)
        ec = {}   
        read_robot_state_op(robot,ec)

    except Exception as e:
        print(e)
