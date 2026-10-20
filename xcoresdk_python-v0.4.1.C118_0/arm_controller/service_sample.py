
import socket

HOST = "0.0.0.0"
PORT = 9000


def handle_client(conn, addr):
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

                    # 在这里编写对应的操作
                    if message == "-1":
                        print("执行 -1 对应的操作")
                    elif message == "0":
                        print("执行 0 对应的操作")
                    elif message == "1":
                        print("执行 1 对应的操作")
                else:
                    print(f"无效消息：{message}")

    except ConnectionResetError:
        print(f"客户端异常断开：{addr}")
    finally:
        conn.close()
        print(f"客户端断开：{addr}")


def main():
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


if __name__ == "__main__":
    main()
