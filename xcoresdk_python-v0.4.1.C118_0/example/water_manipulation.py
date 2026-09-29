import time
import math
import sys
from scipy.spatial.transform import Rotation as R
import threading
import setup_path
import platform
import numpy as np
from my_first_control_file import world_to_robot_coordinates_with_euler,wait_robot
from move_example import wait_robot as wait_single_robot
from datetime import timedelta
import socket
import cv2
import pickle
from force_control_example import calibrate_force_sensor

if platform.system() == 'Windows':
    from Release.windows import xCoreSDK_python
    from Release.windows.xCoreSDK_python.EventInfoKey import MoveExecution
    from Release.windows.xCoreSDK_python import RtSupportedFields
elif platform.system() == 'Linux':
    from Release.linux import xCoreSDK_python
    from Release.linux.xCoreSDK_python.EventInfoKey import MoveExecution
    from Release.linux.xCoreSDK_python import RtSupportedFields
else:
    raise ImportError("Unsupported operating system")
from log import print_log, print_separator
from collisionDetection_example import disable_collision_detection

M_PI = math.pi
#tcpPose_m_l = RtSupportedFields.tcpPose_m
#jointPos_m_l = RtSupportedFields.jointPos_m
#tcpPose_m_r = RtSupportedFields.tcpPose_m
#jointPos_m_r = RtSupportedFields.jointPos_m

def force_cartisian_control(robot,ec):

    fc = robot.forceControl()
    fc.fcInit(xCoreSDK_python.FrameType.flange, ec)
    print_log("fcInit", ec)
    fc.setControlType(1, ec)
    print_log("setControlType", ec)
    fc.setCartesianStiffness([10000000, 10000000, 100000, 5000000, 5000000, 5000000], ec)
    print_log("setCartesianStiffness", ec)
    print("开始笛卡尔模式力控")
    fc.fcStart(ec)
    print_log("fcStart", ec)
    fc.setCartesianDesiredForce([0, 0, -3, 0, 0, 0], ec)
    print_log("setCartesianDesiredForce", ec)




'''
def request_image_from_camera():
    # 创建客户端 Socket
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client_socket.connect(('localhost', 8000))

    # 接收图像数据
    data = b""
    while True:
        packet = client_socket.recv(4096)
        if not packet:
            break
        data += packet

    # 反序列化并解码图像
    buffer = pickle.loads(data)
    image = cv2.imdecode(buffer, cv2.IMREAD_COLOR)

    client_socket.close()
    return image
'''

def receive_image_from_server(port):
    """连接到指定端口的服务器，并接收图像数据"""
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client_socket.connect(('localhost', port))

    data = b""
    while True:
        packet = client_socket.recv(4096)
        if not packet:
            break
        data += packet

    buffer = pickle.loads(data)
    image = cv2.imdecode(buffer, cv2.IMREAD_COLOR)
    client_socket.close()
    return image

def dual_arm_movement(robot_l,robot_r,ec_l,ec_r,point_list_l,point_list_r,move_type):
    if move_type=='absj':
        posl_list = point_list_l
        posr_list = point_list_r
        print("left",posl_list)
        print("right",posr_list)
        cart_pos_l = xCoreSDK_python.CartesianPosition(posl_list)
        cart_pos_r = xCoreSDK_python.CartesianPosition(posr_list)
        robot_model_l = robot_l.model()
        joint_pos_l = robot_model_l.calcIk(cart_pos_l, ec_l)
        robot_model_r = robot_r.model()
        joint_pos_r = robot_model_r.calcIk(cart_pos_r, ec_r)
        absjcmd_l = xCoreSDK_python.MoveAbsJCommand(joint_pos_l, 1000, 0)
        cmdID_l = xCoreSDK_python.PyString()
        absjcmd_r = xCoreSDK_python.MoveAbsJCommand(joint_pos_r, 1000, 0)
        cmdID_r = xCoreSDK_python.PyString()
        print('pause')
        time.sleep(0.1)
        robot_l.moveAppend([absjcmd_l], cmdID_l, ec_l)
        robot_r.moveAppend([absjcmd_r], cmdID_r, ec_r)
        print("Command ID left:", cmdID_l.content())
        print("Command ID right", cmdID_r.content())
        print_log("moveAppend_left", ec_l)
        print_log("moveAppend_right", ec_r)
        robot_l.moveStart(ec_l)
        robot_r.moveStart(ec_r)
        print_log("moveStart", ec_l)
        print_log("moveStart", ec_r)
        wait_robot(robot_l, robot_r, ec_l, ec_r)
    elif move_type=='cartesian':
        #posl_list = world_to_robot_coordinates_with_euler(point_list_l, theta=np.arctan(1 / 3))
        #posr_list = world_to_robot_coordinates_with_euler(point_list_r,bias=[-0.025,0.475,0] ,theta=-np.arctan(1 / 3))
        posl_list = point_list_l
        posr_list = point_list_r
        cart_pos_l = xCoreSDK_python.CartesianPosition(posl_list)
        movelcmd_l = xCoreSDK_python.MoveLCommand(cart_pos_l, 100, 0)
        cart_pos_r = xCoreSDK_python.CartesianPosition(posr_list)
        movelcmd_r = xCoreSDK_python.MoveLCommand(cart_pos_r, 100, 0)
        cmdID_l = xCoreSDK_python.PyString()
        cmdID_r = xCoreSDK_python.PyString()
        #print('pause')
        #time.sleep(0.1)
        robot_l.moveAppend([movelcmd_l], cmdID_l, ec_l)
        robot_r.moveAppend([movelcmd_r], cmdID_r, ec_r)
        print("Command ID:", cmdID_l.content())
        print_log("moveAppend", ec_l)
        print("Command ID:", cmdID_r.content())
        print_log("moveAppend", ec_r)
        robot_l.moveStart(ec_l)
        robot_r.moveStart(ec_r)
        wait_robot(robot_l, robot_r, ec_l, ec_r)
    elif move_type=="joint_space":
        joint_l_list = np.array(point_list_l) * np.pi/180
        joint_r_list = np.array(point_list_r) * np.pi/180
        joint_l_list = list(joint_l_list)
        joint_r_list = list(joint_r_list)
        joint_pos1 = xCoreSDK_python.JointPosition(joint_l_list)
        absjcmd_l = xCoreSDK_python.MoveAbsJCommand(joint_pos1, 1000, 10)
        joint_pos2 = xCoreSDK_python.JointPosition(joint_r_list)
        absjcmd_r = xCoreSDK_python.MoveAbsJCommand(joint_pos2, 1000, 10)
        cmdID_l = xCoreSDK_python.PyString()
        cmdID_r = xCoreSDK_python.PyString()
        robot_l.moveAppend([absjcmd_l],cmdID_l,ec_l)
        robot_r.moveAppend([absjcmd_r],cmdID_r,ec_r)
        print("Command ID:", cmdID_l.content())
        print_log("moveAppend", ec_l)
        print("Command ID:", cmdID_r.content())
        print_log("moveAppend", ec_r)
        robot_l.moveStart(ec_l)
        robot_r.moveStart(ec_r)
        wait_robot(robot_l, robot_r, ec_l, ec_r)
    else:
        print("unkonwn_type")
    return robot_l,robot_r,ec_l,ec_r

def robot_init(ip_l="192.168.0.160",ip_r="192.168.0.160",local_ip1="192.168.2.100",local_ip2="192.168.0.100"):
    robot_l = xCoreSDK_python.xMateRobot(ip_l,local_ip1)  # 左臂
    ec_l = {}
    print_separator("初始化左臂", length=110)
    robot_l.setOperateMode(xCoreSDK_python.OperateMode.manual, ec_l)
    print_log("setOperateMode", ec_l)
    robot_l.setPowerState(True, ec_l)
    print_log("setPowerState", ec_l)
    robot_l.setMotionControlMode(xCoreSDK_python.MotionControlMode.NrtCommandMode, ec_l)
    print_log("setMotionControlMode", ec_l)
    robot_l.setDefaultZone(0, ec_l)
    robot_l.setDefaultSpeed(50, ec_l)
    robot_r = xCoreSDK_python.xMateRobot(ip_r,local_ip2)  # 右臂
    ec_r = {}
    print_separator("初始化右臂", length=110)
    robot_r.setOperateMode(xCoreSDK_python.OperateMode.manual, ec_r)
    print_log("setOperateMode", ec_r)
    robot_r.setPowerState(True, ec_r)
    print_log("setPowerState", ec_r)
    robot_r.setMotionControlMode(xCoreSDK_python.MotionControlMode.NrtCommandMode, ec_r)
    print_log("setMotionControlMode", ec_r)
    robot_r.setDefaultZone(0, ec_r)
    robot_r.setDefaultSpeed(50, ec_r)
    return robot_l,robot_r,ec_l,ec_r

def Arm_offset(robot,direction,distance,which_arm,ec):
    if which_arm=="left":
        theta_trans = np.arctan(1/3)
        bias = [0,0,0]
    elif which_arm=="right":
        theta_trans = -np.arctan(1/3)
        bias = [0,0,0]
    else:
        print("unkown arm")
        return
    cart_posture = robot.cartPosture(xCoreSDK_python.CoordinateType.endInRef, ec)
    pos_list = np.zeros(6)
    trans = np.array(cart_posture.trans)
    rpy = np.array(cart_posture.rpy)
    for i in range(3):
        pos_list[i] = trans[i]
        pos_list[i+3] = rpy[i]
    #pos_list = world_to_robot_coordinates_with_euler(list(pos_list),bias=bias,theta=-theta_trans)
    offset = np.zeros(6)
    if direction == "l":
        offset[1] = distance
    elif direction == "r":
        offset[1] = -distance
    elif direction == "u":
        offset[2] = distance
    elif direction == "d":
        offset[2] = -distance
    elif direction == "f":
        offset[0] = distance
    elif direction == "b":
        offset[0] = -distance
    else:
        print("unkown type")
        return
    pos_list = np.array(pos_list) + offset
    pos_list = list(pos_list)#world_to_robot_coordinates_with_euler(list(pos_list),bias=bias,theta=theta_trans)
    cart_pos = xCoreSDK_python.CartesianPosition(pos_list)
    move_cmd = xCoreSDK_python.MoveLCommand(cart_pos,100,0)
    cmdID = xCoreSDK_python.PyString()
    robot.moveAppend([move_cmd],cmdID,ec)
    robot.moveStart(ec)
    wait_single_robot(robot,ec)

    return robot,ec

def single_arm_movement(robot,ec,point_list,which_arm):
    if which_arm == "left":
        theta_trans = np.arctan(1 / 3)
        bias = [0, 0, 0]
    elif which_arm == "right":
        theta_trans = -np.arctan(1 / 3)
        bias=[-0.025,0.475,0]
    else:
        print("unkown type")
        return
    #point_list = world_to_robot_coordinates_with_euler(point_list, bias=bias, theta=theta_trans)
    cart_pos = xCoreSDK_python.CartesianPosition(point_list)
    robot_model = robot.model()
    joint_pos = robot_model.calcIk(cart_pos, ec)
    #movelcmd = xCoreSDK_python.MoveLCommand(cart_pos, 500, 0)
    #cmdID = xCoreSDK_python.PyString()
    absjcmd = xCoreSDK_python.MoveAbsJCommand(joint_pos, 500, 0)
    cmdID = xCoreSDK_python.PyString()
    robot.moveAppend([absjcmd], cmdID, ec)
    robot.moveStart(ec)
    wait_single_robot(robot,ec)
    return robot,ec

def qurey_cartesian_position_left(robot,ec):#查询左臂末端位姿（世界坐标系）
    cart_posture = robot.cartPosture(xCoreSDK_python.CoordinateType.endInRef, ec)
    pos_list = np.zeros(6)
    trans = np.array(cart_posture.trans)
    rpy = np.array(cart_posture.rpy)
    for i in range(3):
        pos_list[i] = trans[i]
        pos_list[i + 3] = rpy[i]
    #pos_list = world_to_robot_coordinates_with_euler(pos_list, theta=-np.arctan(1 / 3))
    print("left_posture",pos_list)
    return robot,ec,pos_list[0:3]

def qurey_cartesian_position_right(robot,ec):#查询右臂末端位姿（世界坐标系）
    cart_posture = robot.cartPosture(xCoreSDK_python.CoordinateType.endInRef, ec)
    pos_list = np.zeros(6)
    trans = np.array(cart_posture.trans)
    rpy = np.array(cart_posture.rpy)
    for i in range(3):
        pos_list[i] = trans[i]
        pos_list[i + 3] = rpy[i]
    #pos_list = world_to_robot_coordinates_with_euler(pos_list, theta=np.arctan(1 / 3))
    pos_list = np.array(pos_list)
    print("right_posture",pos_list)
    return robot,ec,pos_list[0:3]




def main():
    robot_l,robot_r,ec_l,ec_r = robot_init()
    #robot_l.startReceiveRobotState(timedelta(seconds=1), [tcpPose_m_l, jointPos_m_l])
    #robot_r.startReceiveRobotState(timedelta(seconds=1), [tcpPose_m_r, jointPos_m_r])
    pose_list_l = []
    pose_list_r = []
    joint_list_l = []
    joint_list_r = []
    image_list_top = []
    image_list_front = []
    time_list = []
    running = True
    def move_robot(robot_l,robot_r,ec_l,ec_r):
       robot_l.setDO(0, 0, True, ec_l)
       robot_l.setDO(0, 1, True, ec_l)
       robot_r.setDO(0, 0, True, ec_r)
       robot_r.setDO(0, 1, True, ec_r)

       #回到初始位姿




       init_list_l = [10,45,-45,0,0,0]
       init_list_r = [-10,45,-45,180,0,0]
       robot_l,robot_r,ec_l,ec_r = dual_arm_movement(robot_l,robot_r,ec_l,ec_r,init_list_l,init_list_r,move_type="joint_space")

       #到达预备位置
       list_l = [0.335,0.15,0.50,np.pi/2,0,0]
       list_r = [0.335,-0.35,0.50,np.pi/2,0,np.pi]
       robot_l, robot_r, ec_l, ec_r = dual_arm_movement(robot_l, robot_r, ec_l, ec_r, list_l, list_r,
                                                     move_type="absj")
       robot_l, ec_l,_ = qurey_cartesian_position_left(robot_l, ec_l)
       robot_r, ec_r,_ = qurey_cartesian_position_right(robot_r, ec_r)



       #到达抓取位置
       list_l = [0.402,0.10,0.054,np.pi/2,np.pi / 2,0]
       list_r = [0.402, -0.487, 0.054, np.pi / 2, -np.pi/2, np.pi]
       robot_l, robot_r, ec_l, ec_r = dual_arm_movement(robot_l, robot_r, ec_l, ec_r, list_l, list_r,
                                                     move_type="cartesian")


       #张开夹爪
       print_separator("set_DI", length=80)
       #robot_l.setSimulationMode(True, ec_l)
       #robot_r.setSimulationMode(True, ec_r)
       switch_l1 = robot_l.getDO(0, 0, ec_l)
       switch_l2 = robot_l.getDO(0, 1, ec_l)
       switch_r1 = robot_r.getDO(0, 0, ec_r)
       switch_r2 = robot_r.getDO(0, 1, ec_r)
       print_log("get_DO", ec_l, f"ret={switch_l1,switch_l2}")
       print_log("get_DO", ec_r, f"ret={switch_r1,switch_r2}")
       robot_l.setDO(0,0,True,ec_l)
       robot_l.setDO(0, 1, True, ec_l)
       robot_r.setDO(0,0,True,ec_r)
       robot_r.setDO(0, 1, True, ec_r)
       switch_l = robot_l.getDO(0, 0, ec_l)
       switch_r = robot_r.getDO(0, 0, ec_r)
       print_log("get_DO", ec_l, f"ret={switch_l1, switch_l2}")
       print_log("get_DO", ec_r, f"ret={switch_r1, switch_r2}")

       #disable_collision_detection(robot_l, ec_l)
       #disable_collision_detection(robot_r, ec_r)

       robot_l,ec_l = Arm_offset(robot_l,"r",0.086,"left",ec_l) #左臂向前移动抓取
       robot_r,ec_r = Arm_offset(robot_r,"l",0.136,"right",ec_r)

       #关闭夹爪
       robot_l.setDO(0, 0, False, ec_l)
       robot_l.setDO(0, 1, False, ec_l)
       robot_r.setDO(0, 0, False, ec_r)
       robot_r.setDO(0, 1, False, ec_r)

       #查询位置并向上提
       robot_l,ec_l,pos_l = qurey_cartesian_position_left(robot_l,ec_l)
       robot_r,ec_r,pos_r = qurey_cartesian_position_right(robot_r,ec_r)

       pos_l[2] += 0.1
       pos_r[2] += 0.1
       list_l[0:3] = list(pos_l)
       list_r[0:3] = list(pos_r)
       list_store_l = list_l
       list_store_r = list_r
       robot_l, robot_r, ec_l, ec_r = dual_arm_movement(robot_l, robot_r, ec_l, ec_r, list_l, list_r,
                                                        move_type="cartesian")
       #移动到插座
       list_l = [0.55454389,0.05554042,0.094,np.pi/2,np.pi / 2,0]
       robot_l, robot_r, ec_l, ec_r = dual_arm_movement(robot_l, robot_r, ec_l, ec_r, list_l, list_r,
                                                        move_type="cartesian")
       list_r = [0.55054389, -0.274534042, 0.094, np.pi / 2, -np.pi / 2, np.pi]
       robot_l, robot_r, ec_l, ec_r = dual_arm_movement(robot_l, robot_r, ec_l, ec_r, list_l, list_r,
                                                        move_type="cartesian")
       list_l = [0.55054389, 0.05254042, 0.068, np.pi / 2, np.pi / 2, 0]
       list_r = [0.55054389, -0.272534042, 0.068, np.pi / 2, -np.pi / 2, np.pi]
       robot_l, robot_r, ec_l, ec_r = dual_arm_movement(robot_l, robot_r, ec_l, ec_r, list_l, list_r,
                                                        move_type="cartesian")
       time.sleep(10)
       #取出
       list_l = [0.55054389, 0.05254042, 0.094, np.pi / 2, np.pi / 2, 0]
       list_r = [0.55054389, -0.272534042, 0.094, np.pi / 2, -np.pi / 2, np.pi]
       robot_l, robot_r, ec_l, ec_r = dual_arm_movement(robot_l, robot_r, ec_l, ec_r, list_l, list_r,
                                                        move_type="cartesian")
       # 到达放回位置
       list_r = list_store_r
       robot_l, robot_r, ec_l, ec_r = dual_arm_movement(robot_l, robot_r, ec_l, ec_r, list_l, list_r,
                                                        move_type="cartesian")
       list_l = list_store_l
       robot_l, robot_r, ec_l, ec_r = dual_arm_movement(robot_l, robot_r, ec_l, ec_r, list_l, list_r,
                                                        move_type="cartesian")

       #放回
       list_l[2] = list_l[2] - 0.09
       list_r[2] = list_r[2] - 0.09
       #list_l[0] = list_l[0] - 0.01
       #list_r[0] = list_r[0] - 0.01
       robot_l, robot_r, ec_l, ec_r = dual_arm_movement(robot_l, robot_r, ec_l, ec_r, list_l, list_r,
                                                        move_type="cartesian")
       #打开夹爪

       robot_l.setDO(0, 0, True, ec_l)
       robot_l.setDO(0, 1, True, ec_l)
       robot_r.setDO(0, 0, True, ec_r)
       robot_r.setDO(0, 1, True, ec_r)




       #calibrate_force_sensor(robot_l, ec_l)
       #calibrate_force_sensor(robot_r, ec_r)
       #force_cartisian_control(robot_l, ec_l)
       #force_cartisian_control(robot_r, ec_r)
       #controller_logs = robot_l.queryControllerLog(5, {xCoreSDK_python.LogInfoLevel.error}, ec_l)
       #print_log("queryControllerLog", ec_l)
       #for log in controller_logs:
           #print(log.content)
       #robot_l, ec_l = Arm_offset(robot_l, "r", 0.05, "left", ec_l)  # 左臂向前移动抓取
       #robot_r, ec_r = Arm_offset(robot_r, "l", 0.05, "right", ec_r)
       #list_l = [0.39, -0.17, 0.194, np.pi / 2, 0, 0]
       #list_r = [0.39, -0.217, 0.194, np.pi / 2, 0, np.pi]
       '''
       robot_l, robot_r, ec_l, ec_r = dual_arm_movement(robot_l, robot_r, ec_l, ec_r, list_l, list_r,
                                                        move_type="cartesian")
       '''
       '''
       for i in range(5):
           list_l = [0.39, -0.145, 0, np.pi / 2, 0, 0]
           list_r = [0.39, -0.242, 0, np.pi / 2, 0, np.pi]
           list_l[2] = 0.094 + (i + 1)*0.02
           list_r[2] = 0.094 + (i + 1) * 0.02
           robot_l, robot_r, ec_l, ec_r = dual_arm_movement(robot_l, robot_r, ec_l, ec_r, list_l, list_r,
                                                           move_type="cartesian")
       print("lift_success")

       time.sleep(1)
       for i in range(14):
           list_l = [0.39, -0.145, 0, np.pi / 2, 0, 0]
           list_r = [0.39, -0.242, 0, np.pi / 2, 0, np.pi]
           list_l[0] = 0.39 + (i + 1) * 0.01
           list_r[0] = 0.39 + (i + 1) * 0.01
           list_l[2] = 0.194
           list_r[2] = 0.194
           robot_l, robot_r, ec_l, ec_r = dual_arm_movement(robot_l, robot_r, ec_l, ec_r, list_l, list_r,
                                                            move_type="cartesian")

       for i in range(5):
           list_l = [0.53, -0.145, 0, np.pi / 2, 0, 0]
           list_r = [0.53, -0.242, 0, np.pi / 2, 0, np.pi]
           list_l[2] = 0.194 - (i + 1) * 0.02
           list_r[2] = 0.194 - (i + 1) * 0.02
           robot_l, robot_r, ec_l, ec_r = dual_arm_movement(robot_l, robot_r, ec_l, ec_r, list_l, list_r,
                                                            move_type="cartesian")
       print("move_success")

       list_l = [0.53, -0.0, 0.1, np.pi / 2, 0, 0]
       list_r = [0.53, -0.3, 0.1, np.pi / 2, 0, np.pi]
       robot_l, robot_r, ec_l, ec_r = dual_arm_movement(robot_l, robot_r, ec_l, ec_r, list_l, list_r,
                                                        move_type="cartesian")
       '''








       '''
       #到达倒水位置
       list_l = [0.39,-0.15,0.194,np.pi/2,0,0]
       list_r = [0.39, -0.20, 0.30, 2.34777574, 0, np.pi]
       robot_l, robot_r, ec_l, ec_r = dual_arm_movement(robot_l, robot_r, ec_l, ec_r, list_l, list_r,
                                                     move_type="cartesian")
       #robot_l,ec_l = qurey_cartesian_position_left(robot_l,ec_l)
       #robot_r,ec_r = qurey_cartesian_position_right(robot_r,ec_r)
       #list_r = [ 0.39503332, -0.30941309 , 0.29706684 , 2.34777574 , 0.02913139 , 3.10740927]
       #robot_r, ec_r = single_arm_movement(robot_r, ec_r, list_r, "right")
       '''

       robot_l.stop(ec_l)
       robot_r.stop(ec_r)
       print("stop")
       controller_logs = robot_l.queryControllerLog(5, {xCoreSDK_python.LogInfoLevel.error}, ec_l)
       print_log("queryControllerLog", ec_l)
       for log in controller_logs:
          print(log.content)
    '''
    def read_state(robot_l,robot_r,tcp_list_l,tcp_list_r,joint_list_l,joint_list_r):
        while running:
            tcpPose_l = xCoreSDK_python.PyTypeVectorDouble()
            arr6_l = xCoreSDK_python.PyTypeVectorDouble()
            robot_l.updateRobotState(timedelta(seconds=1))
            robot_l.getStateData(tcpPose_m_l, tcpPose_l, 16)
            robot_l.getStateData(jointPos_m_l, arr6_l, 6)
            tcpPose_r = xCoreSDK_python.PyTypeVectorDouble()
            arr6_r = xCoreSDK_python.PyTypeVectorDouble()
            robot_r.updateRobotState(timedelta(seconds=1))
            robot_r.getStateData(tcpPose_m_r, tcpPose_r, 16)
            robot_r.getStateData(jointPos_m_r, arr6_r, 6)
            tcp_list_l.append(tcpPose_l.content())
            tcp_list_r.append(tcpPose_r.content())
            joint_list_l.append(arr6_l.content())
            joint_list_r.append(arr6_r.content())
            print("TCP pose left:", tcpPose_l.content(), "\nJoint left:", arr6_l.content())
            print("TCP pose right:", tcpPose_r.content(), "\nJoint right:", arr6_r.content())
    '''
    def read_robot_state(robot_l,robot_r,ec_l,ec_r,pos_list_l,pos_list_r,joint_list_l,joint_list_r,image_list_top,image_list_front,time_list):
        time_start = time.time()
        while running:
            if time.time() - time_start >= 0.1:
                time_list.append(time.time() - time_start)
                cart_posture_l = robot_l.cartPosture(xCoreSDK_python.CoordinateType.endInRef, ec_l)
                pos_list_left = np.zeros(6)
                trans_left = np.array(cart_posture_l.trans)
                rpy_left = np.array(cart_posture_l.rpy)
                cart_posture_r = robot_r.cartPosture(xCoreSDK_python.CoordinateType.endInRef, ec_r)
                pos_list_right = np.zeros(6)
                trans_right = np.array(cart_posture_r.trans)
                rpy_right = np.array(cart_posture_r.rpy)
                jointpos_l = robot_l.jointPos(ec_l)
                jointpos_r = robot_r.jointPos(ec_r)
                #image_new = request_image_from_camera()
                image_top = receive_image_from_server(8001)
                image_front = receive_image_from_server(8000)

                for i in range(3):
                    pos_list_left[i] = trans_left[i]
                    pos_list_left[i + 3] = rpy_left[i]
                    pos_list_right[i] = trans_right[i]
                    pos_list_right[i + 3] = rpy_right[i]
                joint_list_l.append(jointpos_l)
                joint_list_r.append(jointpos_r)
                pos_list_l.append(list(pos_list_left))
                pos_list_r.append(list(pos_list_right))
                image_list_top.append(image_top)
                image_list_front.append(image_front)
                print("TCP pose left:", pos_list_left, "\nJoint left:", jointpos_l)
                print("TCP pose right:", pos_list_right, "\nJoint right:", jointpos_r)
                time_start = time.time()



    read_thread = threading.Thread(target=read_robot_state, args=(robot_l,robot_r,ec_l,ec_r,pose_list_l,pose_list_r,joint_list_l,joint_list_r,image_list_top,image_list_front,time_list))
    read_thread.start()
    move_thread = threading.Thread(target=move_robot, args=(robot_l, robot_r, ec_l, ec_r))
    move_thread.start()
    move_thread.join()
    running = False
    read_thread.join()
    time_list = np.array(time_list)
    time_list = np.cumsum(time_list)
    print_separator("disconnect", length=80)
    robot_l.disconnectFromRobot(ec_l)
    robot_r.disconnectFromRobot(ec_r)

    #for i in range(10,30):
        #image = image_list_top[i]
        #cv2.imshow(f'Image {i + 1}', image)
        #cv2.waitKey(0)




if __name__ == "__main__":
    main()


