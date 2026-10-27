# -*- coding: utf-8 -*-
# 协议: [topic][16B SPK1 头][n×12B 脉冲]; SNN 侧 SUB connect 127.0.0.1:5566, 本程序 PUB bind。
import argparse
import struct
import sys
import threading
import time

if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")

MAGIC = 0x53504B31  # "SPK1"
VERSION = 1
DEFAULT_TOPIC = "/robot/input_spikes"
DEFAULT_ENDPOINT = "tcp://*:5566"  # ASYNC_ROBOT_PUB_PORT; SNN 侧 SUB connect 127.0.0.1:5566
HDR = struct.Struct("<IIII")
SPIKE = struct.Struct("<iff")

NEURON_OF_STATE = {-1: 0, 0: 1, 1: 2}  # L/C/R → N0/N1/N2, LOST 不放电
BASE_HZ = 20.0
ERR_GAIN_HZ = 90.0
MAX_HZ = 110.0


def parse_v1(line):
    """一行 V1 包 → dict, 非法返回 None。"""
    if not line.startswith("V1,"):
        return None
    f = line.split(",")
    if len(f) != 9:
        return None
    try:
        return {"seq": int(f[1]), "state": int(f[2]), "err": int(f[3]),
                "cx": int(f[4]), "cy": int(f[5]), "pix": int(f[6])}
    except ValueError:
        return None


def encode_window(state, err_milli, t0, dt, phase):
    """一个输出窗 → [(neuron, time_sec)...]; 相位累积保证平均速率, LOST 静默。"""
    neuron = NEURON_OF_STATE.get(state)
    if neuron is None:
        phase[0] = 0.0
        return []
    hz = min(BASE_HZ + ERR_GAIN_HZ * min(abs(err_milli), 1000) / 1000.0, MAX_HZ)
    phase[0] += hz * dt          # 20Hz×20ms=0.4, 两窗发 1 个, 平均速率不失真
    k = int(phase[0])
    phase[0] -= k
    return [(neuron, t0 + (i + 0.5) * dt / k) for i in range(k)] if k else []


def pack_head(t_step, spike_count):
    return HDR.pack(MAGIC, VERSION, t_step & 0xFFFFFFFF, spike_count)


def pack_body(spikes, basetimestep):
    return b"".join(SPIKE.pack(n, t, basetimestep) for n, t in spikes)


class SerialLatestSource:
    """后台线程持续读串口; 只保留最新 V1 包, 新数据覆盖旧数据。"""
    def __init__(self, port):
        import serial
        self.ser = serial.Serial(port, 115200, timeout=0.2)  # USB CDC 忽略波特率
        self.lock = threading.Lock()
        self.latest = None
        self.frames = 0
        self.malformed = 0
        self.overwritten = 0
        self._stop = False
        self._th = threading.Thread(target=self._run, daemon=True)
        self._th.start()

    def _run(self):
        while not self._stop:
            try:
                line = self.ser.readline().decode("ascii", "ignore").strip()
            except Exception:
                break
            if not line:
                continue
            pkt = parse_v1(line)
            if pkt is None:
                self.malformed += 1
                continue
            with self.lock:
                if self.latest is not None:
                    self.overwritten += 1  # 旧帧还没被取走就被覆盖
                self.latest = pkt
                self.frames += 1

    def take(self):
        with self.lock:
            pkt, self.latest = self.latest, None
            return pkt

    def close(self):
        self._stop = True
        self._th.join(0.5)
        try:
            self.ser.close()
        except Exception:
            pass


class ZmqPublisher:
    def __init__(self, endpoint, topic, bind=True):
        import zmq
        self.sock = zmq.Context.instance().socket(zmq.PUB)
        (self.sock.bind if bind else self.sock.connect)(endpoint)
        self.topic = topic
        self.sent = 0

    def publish(self, t_step, spikes, basetimestep):
        import zmq
        has_body = len(spikes) > 0
        self.sock.send_string(self.topic, flags=zmq.SNDMORE)  # 三帧 multipart, 少 SNDMORE 会变两条独立消息
        self.sock.send(pack_head(t_step, len(spikes)),
                       flags=zmq.SNDMORE if has_body else 0)
        if has_body:
            self.sock.send(pack_body(spikes, basetimestep))
        self.sent += 1


def make_subscriber(endpoint, topic):
    import zmq
    sub = zmq.Context.instance().socket(zmq.SUB)
    sub.connect(endpoint)
    sub.setsockopt_string(zmq.SUBSCRIBE, topic)
    time.sleep(0.5)  # slow joiner: 订阅生效需要时间
    return sub


def verify_receive(sub, topic, n_expect, timeout=3.0):
    """回读逐批校验线格式, 返回通过批数。"""
    import zmq
    ok = 0
    deadline = time.time() + timeout
    while ok < n_expect and time.time() < deadline:
        try:
            frames = sub.recv_multipart(flags=zmq.NOBLOCK)
        except zmq.Again:
            time.sleep(0.02)
            continue
        if len(frames) not in (2, 3) or frames[0].decode("utf-8", "ignore") != topic:
            print("  [BAD] frame count/topic:", len(frames), frames[:1])
            continue
        head = frames[1]
        body = frames[2] if len(frames) == 3 else b""
        magic, ver, t_step, count = HDR.unpack(head)
        if (magic != MAGIC or ver != VERSION or len(head) != HDR.size
                or len(body) != count * SPIKE.size
                or (len(frames) == 2) != (count == 0)):
            print("  [BAD] header/len:", magic, ver, len(head), len(body), count)
            continue
        neurons = set()
        for i in range(count):
            n, t, bts = SPIKE.unpack_from(body, i * SPIKE.size)
            neurons.add(n)
        if not neurons <= set(NEURON_OF_STATE.values()):
            print("  [BAD] neuron id out of range:", neurons)
            continue
        ok += 1
        print("  [OK] step=%-3d spikes=%-2d neurons=%s" % (t_step, count, sorted(neurons)))
    sub.close(0)
    return ok


def main():
    ap = argparse.ArgumentParser(description="V1 串口包 → SNN 脉冲编码翻译程序")
    ap.add_argument("port", nargs="?", default=None)
    ap.add_argument("--loopback", action="store_true")
    ap.add_argument("--stdout", action="store_true")
    ap.add_argument("--endpoint", default=DEFAULT_ENDPOINT)
    ap.add_argument("--topic", default=DEFAULT_TOPIC)
    ap.add_argument("--connect", action="store_true", help="PUB 用 connect(默认 bind)")
    ap.add_argument("--period", type=float, default=0.02, help="输出窗口秒数(默认20ms)")
    ap.add_argument("--repeat", action="store_true", help="loopback 循环输出不退出(长跑测试用)")
    ap.add_argument("--quiet", action="store_true", help="不打印每窗调试行, 只留 DIR 输出")
    ap.add_argument("--snn-dt", type=float, default=0.0001,
                    help="SNN 仿真步长(秒/步, 默认0.1ms); 接收端 Time=(int)(time/basetimestep) 还原步数")
    args = ap.parse_args()
    if not args.loopback and not args.port:
        ap.error("需要串口号或 --loopback")

    scenario = [(-1, 900), (-1, 400), (0, 30), (0, 60), (1, 500),
                (1, 950), (2, 0), (-1, 200), (0, 10), (2, 0)]
    src = None
    if not args.loopback:
        src = SerialLatestSource(args.port)
        print("serial:", args.port, "| 读线程已启动, 最新帧覆盖旧帧")
    else:
        print("loopback: %d synthetic V1 frames @ %.0fms" % (len(scenario), args.period * 1000))
    pub = None
    if not args.stdout:
        pub = ZmqPublisher(args.endpoint, args.topic, bind=not args.connect)
        mode = "(bind)" if not args.connect else "(connect)"
        print("publishing →", args.endpoint, args.topic, mode)
    sub = None
    if args.loopback and not args.stdout:
        sub = make_subscriber(args.endpoint.replace("*", "127.0.0.1"), args.topic)
        print("subscriber ready")

    n = bad = 0
    t_step = 0
    total_spikes = 0
    hist = {0: 0, 1: 0, 2: 0}
    phase = [0.0]
    last_pkt = None
    base_t = None  # 真机模式: 首包到达时刻=仿真时间 0 点
    NAME = {0: "N0/LEFT", 1: "N1/CENTER", 2: "N2/RIGHT"}
    next_tick = time.perf_counter()
    try:
        while True:
            wait = next_tick - time.perf_counter()
            if wait > 0:
                time.sleep(wait)
            next_tick += args.period
            if args.loopback:
                if not args.repeat and n >= len(scenario):
                    break
                st, err = scenario[n % len(scenario)]
                pkt = {"seq": n, "state": st, "err": err}
                now = t_step * args.period
            else:
                pkt = src.take()
                if pkt is None:
                    pkt = last_pkt  # 本窗无新帧: 沿用上次状态
                if pkt is None:
                    continue
                raw = time.perf_counter()
                if base_t is None:
                    base_t = raw
                now = raw - base_t
            last_pkt = pkt
            spikes = encode_window(pkt["state"], pkt["err"], now, args.period, phase)
            n += 1
            t_step += 1
            total_spikes += len(spikes)
            for sp in spikes:
                hist[sp[0]] += 1
            if pub is not None:
                pub.publish(t_step, spikes, args.snn_dt)  # basetimestep=SNN步长, 不是窗口长度
            if not args.quiet:
                tgt = NAME.get(NEURON_OF_STATE.get(pkt["state"], -1), "LOST/静默")
                print("#%-6d state=%+d %-9s → %-9s %2d spikes  cum: %d批/%d脉"
                      % (pkt["seq"], pkt["state"], "LOST" if pkt["state"] == 2 else "", tgt,
                         len(spikes), t_step, total_spikes))
            if args.stdout:
                # 给外部 C++ 读的干净行(对方 input_output.cpp 用 _popen + sscanf("DIR,%d"))
                print("DIR,%d" % pkt["state"], flush=True)
    except (KeyboardInterrupt, OSError):
        # OSError: 父进程(_popen 调用方)关闭管道/退出时 stdout 写入失败, 干净退出即可
        pass
    if src is not None:
        bad = src.malformed
        ow = src.overwritten
        src.close()
    else:
        ow = 0
    try:
        print("\ndone: %d windows (%d malformed), %d batches, %d spikes  hist=%s  overwritten=%d"
              % (n, bad, t_step, total_spikes, hist, ow))
    except OSError:
        pass  # 管道已被父进程关闭

    if sub is not None:
        print("\nroundtrip verify: receiving back... (expect %d batches)" % pub.sent)
        ok = verify_receive(sub, args.topic, pub.sent)
        print("roundtrip:", "PASS" if ok == pub.sent else "FAIL (%d/%d)" % (ok, pub.sent))


if __name__ == "__main__":
    main()
