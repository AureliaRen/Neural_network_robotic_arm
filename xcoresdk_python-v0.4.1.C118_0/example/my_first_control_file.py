import time
import math
import sys
import setup_path
import platform
import numpy as np

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

def wait_robot(robot_l,robot_r,ec_l,ec_r):
    '''等待运动结束 - 通过查询机械臂是否处于运动中的方式'''
    print_separator("wait_robot",length=80)
    running = True
    while (running):
        time.sleep(0.1)
        st_l = robot_l.operationState(ec_l)
        st_r = robot_r.operationState(ec_r)
        if((st_l == xCoreSDK_python.OperationState.idle and st_r == xCoreSDK_python.OperationState.idle) or st_l == xCoreSDK_python.OperationState.unknown or st_r == xCoreSDK_python.OperationState.unknown):
            running = False
    print("move finished")


from scipy.spatial.transform import Rotation as R

def world_to_robot_coordinates_with_euler(car_cor,bias=[0,0,0],theta=np.arctan(1/3)):
    """
    Convert world coordinates and Euler angles to robot coordinates and Euler angles
    accounting for a rotation around the z-axis by arctan(1/3).

    Args:
        world_coords (tuple or list): Position in the world frame (x, y, z).
        world_euler (tuple or list): Euler angles in the world frame (roll, pitch, yaw) in radians.

    Returns:
        tuple: Robot frame position (x, y, z) and Euler angles (roll, pitch, yaw) in radians.
    """
    # Rotation angle (in radians)
    #theta = np.arctan(1 / 3)
    world_coords = list(np.array(car_cor[0:3]) + np.array(bias))
    world_euler = car_cor[3:6]

    # Rotation matrix around z-axis
    rotation_matrix = np.array([
        [np.cos(theta), -np.sin(theta), 0],
        [np.sin(theta),  np.cos(theta), 0],
        [0,              0,             1]
    ])

    # Convert input position to numpy array
    world_coords = np.array(world_coords)

    # Validate position input dimension
    if world_coords.shape != (3,):
        raise ValueError("Position input must be a 3D coordinate (x, y, z).")

    # Apply rotation to position
    robot_coords = rotation_matrix @ world_coords
    robot_coords = robot_coords

    # Convert Euler angles to rotation matrix
    world_rotation = R.from_euler('xyz', world_euler)

    # Combine world rotation with z-axis rotation
    z_axis_rotation = R.from_matrix(rotation_matrix)
    robot_rotation = z_axis_rotation * world_rotation

    # Convert robot rotation back to Euler angles
    robot_euler = robot_rotation.as_euler('xyz')
    rob_cor = np.zeros(6)
    for i in range(3):
        rob_cor[i] = robot_coords[i]
        rob_cor[i+3] = robot_euler[i]



    return rob_cor

def main():
    ip_l = "192.168.0.160"
    robot_l = xCoreSDK_python.xMateRobot(ip_l) #左臂
    ec_l = {}
    print_separator("初始化左臂", length=110)
    robot_l.setOperateMode(xCoreSDK_python.OperateMode.manual, ec_l)
    print_log("setOperateMode",ec_l)
    robot_l.setPowerState(True, ec_l)
    print_log("setPowerState",ec_l)
    robot_l.setMotionControlMode(xCoreSDK_python.MotionControlMode.NrtCommandMode, ec_l)
    print_log("setMotionControlMode",ec_l)
    robot_l.setDefaultZone(0, ec_l)
    robot_l.setDefaultSpeed(50,ec_l)
    ip_r = "192.168.0.160"
    robot_r = xCoreSDK_python.xMateRobot(ip_r)  # 左臂
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
    #初始复位

    print_separator("初始复位")
    posl_list = [0.235,0.15,0.50,-np.pi/2,0,np.pi]
    posl_list = world_to_robot_coordinates_with_euler(posl_list,theta=np.arctan(1/3))
    posr_list = [0.235,0.15,0.50,np.pi/2,0,np.pi]
    posr_list = world_to_robot_coordinates_with_euler(posr_list,theta=-np.arctan(1/3))
    cart_pos_l = xCoreSDK_python.CartesianPosition(posl_list)
    cart_pos_r = xCoreSDK_python.CartesianPosition(posr_list)
    robot_model_l = robot_l.model()
    joint_pos_l = robot_model_l.calcIk(cart_pos_l, ec_l)
    robot_model_r = robot_l.model()
    joint_pos_r = robot_model_r.calcIk(cart_pos_r, ec_r)
    absjcmd_l = xCoreSDK_python.MoveAbsJCommand(joint_pos_l, 1000, 0)
    cmdID_l = xCoreSDK_python.PyString()
    absjcmd_r = xCoreSDK_python.MoveAbsJCommand(joint_pos_r, 1000, 0)
    cmdID_r = xCoreSDK_python.PyString()
    print('pause')
    time.sleep(2)
    robot_l.moveAppend([absjcmd_l], cmdID_l, ec_l)  # [absjcmd]指令列表，可以添加多条指令，须为同类型指令
    robot_r.moveAppend([absjcmd_r],cmdID_r,ec_r)
    print("Command ID left:", cmdID_l.content())
    print("Command ID right", cmdID_r.content())
    print_log("moveAppend_left", ec_l)
    print_log("moveAppend_right",ec_r)
    robot_l.moveStart(ec_l)
    robot_r.moveStart(ec_r)
    print_log("moveStart", ec_l)
    print_log("moveStart", ec_r)
    wait_robot(robot_l,robot_r,ec_l,ec_r)


    #运动到任务笛卡尔点位
    print_separator("运动到目标点", length=80)
    pos2_list_l = [0.345,0.10,0.234,-np.pi/2,0,np.pi]
    pos2_list_l = world_to_robot_coordinates_with_euler(pos2_list_l,theta=np.arctan(1/3))
    pos2_list_r = [0.345, 0.187, 0.234, np.pi / 2, 0, np.pi]
    pos2_list_r = world_to_robot_coordinates_with_euler(pos2_list_r, theta=-np.arctan(1 / 3))
    cart_pos_l = xCoreSDK_python.CartesianPosition(pos2_list_l)
    movelcmd_l = xCoreSDK_python.MoveLCommand(cart_pos_l, 500, 0)
    cart_pos_r = xCoreSDK_python.CartesianPosition(pos2_list_r)
    movelcmd_r = xCoreSDK_python.MoveLCommand(cart_pos_r, 500, 0)
    cmdID_l = xCoreSDK_python.PyString()
    cmdID_r = xCoreSDK_python.PyString()
    print('pause')
    time.sleep(2)
    robot_l.moveAppend([movelcmd_l], cmdID_l, ec_l)
    robot_r.moveAppend([movelcmd_r], cmdID_r, ec_r)
    print("Command ID:", cmdID_l.content())
    print_log("moveAppend", ec_l)
    print("Command ID:", cmdID_r.content())
    print_log("moveAppend", ec_r)
    robot_l.moveStart(ec_l)
    robot_r.moveStart(ec_r)
    wait_robot(robot_l,robot_r, ec_l,ec_r)
    robot_l.stop(ec_l)
    robot_r.stop(ec_r)
    print("stop")
    print_separator("query controller log", length=80)
    # 查询最近5条错误级别控制器日志
    controller_logs = robot_l.queryControllerLog(5, {xCoreSDK_python.LogInfoLevel.error}, ec_l)
    print_log("queryControllerLog", ec_l)
    for log in controller_logs:
        print(log.content)
    print_separator("disconnect", length=80)
    robot_l.disconnectFromRobot(ec_l)
    robot_l.disconnectFromRobot(ec_r)


if __name__ == "__main__":
    main()


