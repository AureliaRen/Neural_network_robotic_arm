
import platform
from sparkai_text_trans import nlp_translator
import socket
from water_manipulation import Arm_offset,robot_init

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


def server(robot_l,ec_l,robot_r,ec_r):
    # 创建一个TCP/IP socket
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    # 获取主机和端口
    host = 'localhost'
    port = 12345

    # 绑定到地址
    server_socket.bind((host, port))

    # 开始监听连接，最大排队数为5
    server_socket.listen(5)
    print(f"服务器启动，监听 {host}:{port}...")

    while True:
        # 接受客户端连接
        client_socket, client_address = server_socket.accept()
        print(f"客户端 {client_address} 已连接")

        try:
            while True:  # 保持与客户端的连接，直到客户端关闭
                # 接收客户端数据
                data = client_socket.recv(1024)
                if not data:
                    print("客户端已断开连接")
                    break  # 客户端断开连接，退出接收循环

                print(f"接收到数据: {data.decode()}")

                # 返回应答
                response = f"服务器收到: {data.decode()}"
                client_socket.sendall(response.encode())
                receive_text = data.decode()
                if receive_text:
                    if "爪" in receive_text or "夹" in receive_text: #判断是否为夹爪指令
                        command_list = nlp_translator(receive_text,True)
                        robot_l,ec_l,robot_r,ec_r = gripper_action(command_list,robot_l,ec_l,robot_r,ec_r)

                    else:
                        command_list = nlp_translator(receive_text)
                        robot_l,ec_l,robot_r,ec_r = move_according_to_command(command_list,robot_l,ec_l,robot_r,ec_r)


        except Exception as e:
            print(f"处理客户端请求时出错: {e}")

        finally:
            # 关闭与客户端的连接
            client_socket.close()
'''
def move_according_to_command(receive_text,robot,ec):
    if "左" in receive_text:
        print("向左移动")
        robot,ec = Arm_offset(robot,"l",0.05,"left",ec)
    elif "右" in receive_text:
        robot, ec = Arm_offset(robot, "r", 0.05, "left", ec)
    elif "上" in receive_text:
        robot, ec = Arm_offset(robot, "u", 0.05, "left", ec)
    elif "下" in receive_text:
        robot, ec = Arm_offset(robot, "d", 0.05, "left", ec)
    elif "前" in receive_text:
        robot, ec = Arm_offset(robot, "f", 0.05, "left", ec)
    elif "后" in receive_text:
        robot, ec = Arm_offset(robot, "b", 0.05, "left", ec)
    else:
        print("没有收到有效移动指令")
    return robot,ec
'''
def move_according_to_command(receive_text,robot_l,ec_l,robot_r,ec_r):
    direction = receive_text[1]
    distance = receive_text[2]
    which_arm = receive_text[0]
    if not "None" in direction and not "None" in distance and not "None" in which_arm:
        distance = float(distance)
        if "左" in which_arm:
            if "左" in direction:
                robot_l, ec_l = Arm_offset(robot_l, "l", distance, "left", ec_l)
            elif "右" in direction:
                robot_l, ec_l = Arm_offset(robot_l, "r", distance, "left", ec_l)
            elif "上" in direction:
                robot_l, ec_l = Arm_offset(robot_l, "u", distance, "left", ec_l)
            elif "下" in direction:
                robot_l, ec_l = Arm_offset(robot_l, "d", distance, "left", ec_l)
            elif "前" in direction:
                robot_l, ec_l = Arm_offset(robot_l, "f", distance, "left", ec_l)
            elif "后" in direction:
                robot_l, ec_l = Arm_offset(robot_l, "b", distance, "left", ec_l)
            else:
                print("没有收到有效移动指令")
        elif "右" in which_arm:
            if "左" in direction:
                robot_r, ec_r = Arm_offset(robot_r, "l", distance, "right", ec_r)
            elif "右" in direction:
                robot_r, ec_r = Arm_offset(robot_r, "r", distance, "right", ec_r)
            elif "上" in direction:
                robot_r, ec_r = Arm_offset(robot_r, "u", distance, "right", ec_r)
            elif "下" in direction:
                robot_r, ec_r = Arm_offset(robot_r, "d", distance, "right", ec_r)
            elif "前" in direction:
                robot_r, ec_r = Arm_offset(robot_r, "f", distance, "right", ec_r)
            elif "后" in direction:
                robot_r, ec_r = Arm_offset(robot_r, "b", distance, "right", ec_r)
            else:
                print("没有收到有效移动指令")

        return robot_l,ec_l,robot_r,ec_r
    else:
        print("无效的输入")

def gripper_action(recieve_text,robot_l,ec_l,robot_r,ec_r): #根据动作指令执行夹爪动作
    action = "unkown"
    if "开" in recieve_text[1] or "张" in recieve_text[1]:
        print("张开夹爪")
        action = "open"
    elif "关" in recieve_text[1] or "合" in recieve_text[1] or "闭" in recieve_text[1]:
        print("闭合夹爪")
        action = "close"
    if "左" in recieve_text[0]:
        if action == "open":
            robot_l.setDO(0,0,True,ec_l)
            robot_l.setDO(0, 1, True, ec_l)
        elif action == "close":
            robot_l.setDO(0,0,False,ec_l)
            robot_l.setDO(0, 1, False, ec_l)
        else:
            print("unkown action")
    elif "右" in recieve_text[0]:
        if action == "open":
            robot_r.setDO(0,0,True,ec_r)
            robot_r.setDO(0, 1, True, ec_r)
        elif action == "close":
            robot_r.setDO(0,0,False,ec_r)
            robot_r.setDO(0, 1, False, ec_r)
        else:
            print("unkown action")
    return robot_l,ec_l,robot_r,ec_r


def main():
    '''
    ip = '192.168.0.160'
    local_ip = '192.168.2.100'
    robot = xCoreSDK_python.xMateRobot(ip, local_ip)
    ec = {}
    print_separator("初始化左臂", length=110)
    robot.setOperateMode(xCoreSDK_python.OperateMode.manual, ec)
    print_log("setOperateMode", ec)
    robot.setPowerState(True, ec)
    print_log("setPowerState", ec)
    robot.setMotionControlMode(xCoreSDK_python.MotionControlMode.NrtCommandMode, ec)
    print_log("setMotionControlMode", ec)
    robot.setDefaultZone(0, ec)
    robot.setDefaultSpeed(50, ec)
    '''
    robot_l, robot_r, ec_l, ec_r = robot_init()
    server(robot_l,ec_l,robot_r,ec_r)







if __name__ == "__main__":
    main()


