import time
import threading
import platform
import numpy as np
import cv2
from water_manipulation import Arm_offset, receive_image_from_server

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
'''用于简单运动轨迹下的数据采集测试'''
def single_arm_movement(robot_r,ec_r,point_list_r,move_type):
    if move_type=='absj':
        posr_list = point_list_r
        print("right",posr_list)
        cart_pos_r = xCoreSDK_python.CartesianPosition(posr_list)
        robot_model_r = robot_r.model()
        joint_pos_r = robot_model_r.calcIk(cart_pos_r, ec_r)
        absjcmd_r = xCoreSDK_python.MoveAbsJCommand(joint_pos_r, 1000, 0)
        cmdID_r = xCoreSDK_python.PyString()
        print('pause')
        time.sleep(0.1)
        robot_r.moveAppend([absjcmd_r], cmdID_r, ec_r)
        print("Command ID right", cmdID_r.content())
        print_log("moveAppend_right", ec_r)
        robot_r.moveStart(ec_r)
        print_log("moveStart", ec_r)
        wait_robot_single(robot_r, ec_r)
    elif move_type=='cartesian':
        #posl_list = world_to_robot_coordinates_with_euler(point_list_l, theta=np.arctan(1 / 3))
        #posr_list = world_to_robot_coordinates_with_euler(point_list_r,bias=[-0.025,0.475,0] ,theta=-np.arctan(1 / 3))
        posr_list = point_list_r
        cart_pos_r = xCoreSDK_python.CartesianPosition(posr_list)
        movelcmd_r = xCoreSDK_python.MoveLCommand(cart_pos_r, 100, 0)
        cmdID_r = xCoreSDK_python.PyString()
        #print('pause')
        #time.sleep(0.1)
        robot_r.moveAppend([movelcmd_r], cmdID_r, ec_r)
        print("Command ID:", cmdID_r.content())
        print_log("moveAppend", ec_r)
        robot_r.moveStart(ec_r)
        wait_robot_single(robot_r, ec_r)
    elif move_type=="joint_space":
        joint_r_list = np.array(point_list_r) * np.pi/180
        joint_r_list = list(joint_r_list)
        joint_pos2 = xCoreSDK_python.JointPosition(joint_r_list)
        absjcmd_r = xCoreSDK_python.MoveAbsJCommand(joint_pos2, 1000, 10)
        cmdID_r = xCoreSDK_python.PyString()
        robot_r.moveAppend([absjcmd_r],cmdID_r,ec_r)
        print("Command ID:", cmdID_r.content())
        print_log("moveAppend", ec_r)
        robot_r.moveStart(ec_r)
        wait_robot_single(robot_r, ec_r)
    else:
        print("unkonwn_type")
    return robot_r,ec_r

def wait_robot_single(robot_r,ec_r):
    '''等待运动结束 - 通过查询机械臂是否处于运动中的方式'''
    print_separator("wait_robot",length=80)
    running = True
    while (running):
        time.sleep(0.1)
        st_r = robot_r.operationState(ec_r)
        if((st_r == xCoreSDK_python.OperationState.idle) or  st_r == xCoreSDK_python.OperationState.unknown):
            running = False
    print("move finished")


def robot_init_single(ip_r="192.168.0.160", local_ip2="192.168.2.100"):
    robot_r = xCoreSDK_python.xMateRobot(ip_r, local_ip2)  # 右臂
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
    return  robot_r, ec_r

def main():
    robot_r, ec_r = robot_init_single()
    pos_list_r = []
    joint_list_r = []
    image_list_top = []
    image_list_wist = []
    time_list = []
    running = True
    def move_robot(robot_r,ec_r):
        global gripper_state
        robot_r.setDO(0, 0, True, ec_r)
        robot_r.setDO(0, 1, True, ec_r) #张开夹爪
        gripper_state = 0
        init_list_r = [10,45,-45,0,0,0]
        robot_r, ec_r = single_arm_movement(robot_r, ec_r, init_list_r, move_type="joint_space") #回到初始位姿
        list_r = [0.335,0.15,0.50,np.pi/2,0,0]
        robot_r, ec_r = single_arm_movement(robot_r, ec_r, list_r, move_type="absj") #到达抓取位置
        list_r = [0.402, 0.10, 0.054, np.pi / 2, np.pi / 2, 0]
        robot_r, ec_r = single_arm_movement(robot_r, ec_r, list_r, move_type="cartesian")
        #robot_r.setDO(0, 0, True, ec_r)
        #robot_r.setDO(0, 1, True, ec_r) #张开夹爪
        #gripper_state = 0
        #robot_r, ec_r = Arm_offset(robot_r, "r", 0.036, "right", ec_r) #右臂向前移动抓取
        #robot_r.setDO(0, 0, False, ec_r)
        #robot_r.setDO(0, 1, False, ec_r) #关闭夹爪
        #gripper_state = 1
        #robot_r, ec_r = Arm_offset(robot_r, "l", 0.036, "right", ec_r)
        robot_r.stop(ec_r)
        print("stop")
        controller_logs = robot_r.queryControllerLog(5, {xCoreSDK_python.LogInfoLevel.error}, ec_r)
        print_log("queryControllerLog", ec_r)
        for log in controller_logs:
            print(log.content)

    def read_robot_state(robot_r, ec_r, pos_list_r, joint_list_r,
                             image_list_top, image_list_wist, time_list):
        path = "F:/datasets/my_first_task/1/"
        time_start = time.time()
        while running:
            if time.time() - time_start >= 0.1:
                time_delta = time.time() - time_start
                time_list.append(time.time() - time_start)
                cart_posture_r = robot_r.cartPosture(xCoreSDK_python.CoordinateType.endInRef, ec_r)
                pos_list_right = np.zeros(6)
                trans_right = np.array(cart_posture_r.trans)
                rpy_right = np.array(cart_posture_r.rpy)
                jointpos_r = robot_r.jointPos(ec_r)
                # image_new = request_image_from_camera()
                image_top = receive_image_from_server(8001)
                image_wist = receive_image_from_server(8000)

                for i in range(3):
                    pos_list_right[i] = trans_right[i]
                    pos_list_right[i + 3] = rpy_right[i]
                joint_list_r.append(jointpos_r)
                pos_list_r.append(list(pos_list_right))
                image_list_top.append(image_top)
                image_list_wist.append(image_wist)
                print("TCP pose right:", pos_list_right, "\nJoint right:", jointpos_r, "\nTime", time_delta)
                data = {
                        'joint' : np.array(jointpos_r, dtype=np.float32),
                        'pose' : np.array(pos_list_right, dtype=np.float32),
                        'image' : np.array(image_top),
                        'wrist_image' : np.array(image_wist),
                        'depth_image' : np.array(image_wist),
                        'gripper' : gripper_state
                }
                index = len(time_list)
                cv2.imwrite(path+str(index)+'.jpg', image_top)
                cv2.imwrite(path+str(index)+'_wist.jpg', image_wist)
                np.save(path+"targ"+str(index), data)
                time_start = time.time()

    read_thread = threading.Thread(target=read_robot_state, args=(
    robot_r, ec_r, pos_list_r, joint_list_r, image_list_top,
    image_list_wist, time_list))
    read_thread.start()
    move_thread = threading.Thread(target=move_robot, args=(robot_r, ec_r))
    move_thread.start()
    move_thread.join()
    running = False
    read_thread.join()
    time_list = np.array(time_list)
    time_list = np.cumsum(time_list)
    print_separator("disconnect", length=80)
    robot_r.disconnectFromRobot(ec_r)
if __name__ == "__main__":
    main()





