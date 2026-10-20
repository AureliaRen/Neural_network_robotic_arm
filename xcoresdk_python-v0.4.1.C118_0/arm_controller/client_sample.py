
import socket

SERVER_IP = "127.0.0.1"  # 修改为接收端 PC 的实际 IP
SERVER_PORT = 9000


def main():
    with socket.create_connection(
        (SERVER_IP, SERVER_PORT), timeout=5
    ) as sock:
        print(f"已连接到 {SERVER_IP}:{SERVER_PORT}")

        while True:
            message = input("请输入 -1、0 或 1，输入 q 退出：").strip()

            if message.lower() == "q":
                break

            if message not in ("-1", "0", "1"):
                print("输入无效，只能发送 -1、0 或 1")
                continue

            sock.sendall((message + "\n").encode("utf-8"))
            print(f"已发送：{message}")


if __name__ == "__main__":
    main()
