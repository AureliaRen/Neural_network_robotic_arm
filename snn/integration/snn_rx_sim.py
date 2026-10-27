# -*- coding: utf-8 -*-
# snn_rx_sim.py —— 模拟 net_package ZMQAsyncInputOutputSpikeDriver 的接收路径
# 逐行复现 C++ 侧逻辑(ZMQAsyncInputOutputSpikeDriver.cpp):
#   pollIncomingBatches(): 收 [topic][16B SPK1 头][n×12B 脉冲]
#   LoadInputSpike():      Time = (int)(spike.time / spike.basetimestepl)  → 绝对步数
# 用途: 无 C++/ZeroMQ 构建环境时验证 v1_to_spikes 的时间戳语义与批格式。
# 用法: python snn_rx_sim.py [--endpoint tcp://127.0.0.1:5566] [--topic /robot/input_spikes]
#                            [--dt 0.0001] [--speed 1.0] [--timeout 30]
#   --dt     SNN 的 basetimestep(秒/步), 仅用于判断事件相对"仿真当前步"的领先/滞后
#   --speed  仿真速度倍率(1.0=实时): 当前步 = 真实经过秒数 × speed / dt
import argparse
import struct
import sys
import time

if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")

MAGIC = 0x53504B31  # "SPK1"
VERSION = 1
HDR = struct.Struct("<IIII")
SPIKE = struct.Struct("<iff")  # i32 neuron, f32 time(秒), f32 basetimestep(秒/步)


def main():
    ap = argparse.ArgumentParser(description="模拟 SNN Async ZMQ 接收端")
    ap.add_argument("--endpoint", default="tcp://127.0.0.1:5566")
    ap.add_argument("--topic", default="/robot/input_spikes")
    ap.add_argument("--dt", type=float, default=0.0001, help="basetimestep 秒/步")
    ap.add_argument("--speed", type=float, default=1.0, help="仿真速度倍率")
    ap.add_argument("--timeout", type=float, default=30.0)
    args = ap.parse_args()

    import zmq
    ctx = zmq.Context.instance()
    sub = ctx.socket(zmq.SUB)
    sub.connect(args.endpoint)
    sub.setsockopt_string(zmq.SUBSCRIBE, args.topic)
    time.sleep(0.3)  # slow joiner
    print("SUB connect", args.endpoint, "topic", args.topic,
          "| dt=%.6fs speed=%.2f" % (args.dt, args.speed))
    print("waiting for batches (Ctrl+C to stop)...")

    t_start = None
    n_batch = 0
    n_spike = 0
    bad = 0
    deadline = time.time() + args.timeout

    try:
        while time.time() < deadline:
            if not sub.poll(100):
                continue
            frames = sub.recv_multipart()
            now = time.perf_counter()
            if t_start is None:
                t_start = now
                cur_step = 0
            else:
                cur_step = int((now - t_start) * args.speed / args.dt)

            if len(frames) not in (2, 3):
                print("[BAD] frame count:", len(frames))
                bad += 1
                continue
            topic = frames[0].decode("utf-8", "ignore")
            head = frames[1]
            body = frames[2] if len(frames) == 3 else b""  # 空批只有 topic+header 两帧
            magic, ver, t_step, count = HDR.unpack(head)
            if topic != args.topic or magic != MAGIC or ver != VERSION:
                print("[BAD] topic/magic/ver:", topic, hex(magic), ver)
                bad += 1
                continue
            if len(body) != count * SPIKE.size:
                print("[BAD] body len:", len(body), "expect", count * SPIKE.size)
                bad += 1
                continue

            n_batch += 1
            n_spike += count
            rows = []
            for i in range(count):
                neuron, t_s, bts = SPIKE.unpack_from(body, i * SPIKE.size)
                step = int(t_s / bts) if bts else -1  # ← C++: (int)(spike.time / spike.basetimestepl)
                rows.append((neuron, t_s, bts, step))

            if rows:
                lead = rows[-1][3] - cur_step
                print("batch#%-3d hdr_step=%-4d spikes=%-2d | 事件步 %d..%d | 仿真当前步 %d | 领先 %+d 步"
                      % (n_batch, t_step, count, rows[0][3], rows[-1][3], cur_step, lead))
                for neuron, t_s, bts, step in rows:
                    print("    neuron=%-2d time=%.6fs bts=%.6fs → step=%d" % (neuron, t_s, bts, step))
            else:
                print("batch#%-3d hdr_step=%-4d spikes=0 (空批)" % (n_batch, t_step))
    except KeyboardInterrupt:
        pass

    print("\ndone: %d batches, %d spikes, %d bad" % (n_batch, n_spike, bad))
    print("verdict:", "PASS" if (n_batch > 0 and bad == 0) else "FAIL/NO DATA")


if __name__ == "__main__":
    main()
