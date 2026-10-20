
import socket
import init
import time
import math
import sys
import setup_path
import platform
import numpy as np
import math
import move_example as example
HOST = "0.0.0.0"
PORT = 9000
import arm_controller
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
def handle_client(conn, addr ):
    print(f"PC 已连接：{addr}")
    buffer = b""

    try:
        while True:
            data = conn.recv(1024)
            if not data:
                break

            buffer += data

            while b"\n" in buffer:
                line, buffer = buffer.split(b"\n", 1)
                message = line.decode("utf-8").strip()

                if message in ("-1", "0", "1"):
                    print(f"收到来自 {addr[0]} 的消息：{message}")
                    arm_controller.end_move(robot,ec,message)
                else:
                    print(f"无效消息：{message}")

    except ConnectionResetError:
        print(f"客户端异常断开：{addr}")
    finally:
        conn.close()
        print(f"客户端断开：{addr}")


def main():
     try:
            ip = "192.168.0.160"
            robot = xCoreSDK_python.xMateRobot(ip)
            ec = {}
            print_separator("move_op", length=110)
            init.pre_op(robot, ec)
            example.get_joint_pos(robot, ec)
            # example.print_move_info(example.get_move_info(robot, ec))
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server:
                server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                server.bind((HOST, PORT))
                server.listen()

                print(f"监听端口：{PORT}")

                while True:
                    conn, addr = server.accept()
                    import threading
                    threading.Thread(
                    target=handle_client,
                    args=(conn, addr),
                    daemon=True
                    ).start()
            robot.setNoneEventWatcher(xCoreSDK_python.Event.moveExecution, ec)  # 结束时需要关闭监视器
            print("joint_pos")
            example.get_joint_pos(robot, ec)
            print("cart_posture")
            example.get_cart_posture(robot, ec)
     except Exception as e:
            print(f"An error occurred: {e}")



if __name__ == "__main__":
    main()
