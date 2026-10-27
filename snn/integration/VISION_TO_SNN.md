> **仓库位置**：本文档随视觉线发送端一起提交在 `snn/integration/`。
> - 发送端：`v1_to_spikes.py`（OpenMV V1 串口包 → 三神经元脉冲 → ZMQ / DIR 行）
> - 接收验证：`snn_rx_sim.py`（复现 SNN 侧收包与步数换算，不依赖 C++）
> - C++ 收包示例：`spike_rx_demo.cpp`
>
> 说明：文档原写于本地 net_package 工作区（`D:\snnplatform\net_package-main`），
> 其中 `mainprogram/zmq_link_check.cpp`、`build_zmq` 等属本地验证环境，不在本仓库；
> SNN 侧接入按 §一 / §五 的约定即可。

# v1_to_spikes 对接说明（给 SNN / 机械臂侧）

> 代码：`v1_to_spikes.py`（串口翻译程序，Python 3）
> 作用：OpenMV 相机 V1 串口包 → 三神经元脉冲编码 → ZMQ 发布给 net_package SNN
> 运行方：视觉线（拿相机的人）。**接收方（SNN 侧）只需要按下面的约定改一处 SNN 程序。**

## 一、你要改的（SNN 侧，一处）

在 SNN 的 main 程序里加异步 ZMQ 驱动：

```cpp
mysim->AddZMQAsyncInputOutputSpikeDriver(
    "127.0.0.1",
    SNN_PUB_PORT,            // 你的 SNN 输出端口，自己定（示例 5567）
    5566,                    // 机器人输入端口：固定 5566，与 v1_to_spikes 对齐
    "/snn/output_spikes",
    "/robot/input_spikes",
    10);                     // 通信间隔（步）
```

参考现成写法：`ZMQcommunication_test.cpp:325`。

## 一·B、更简单的路：直接读 Python 的 stdout

> SNN 侧用 `_popen` 启动 Python 子进程、逐行读 `DIR,<state>` 即可，不需要 ZMQ。
> 已验证：loopback 24 决策轮 + 真机 COM4 50 决策轮，方向跟随正确。
> 用法：在工作目录下运行 `python -u v1_to_spikes.py <参数> --stdout --repeat`，`<参数>` 为 `--loopback` 或串口号（如 COM4）。

SNN 侧读取方式（示例）：

```cpp
"python.exe" -u "v1_to_spikes.py" --loopback --stdout --repeat
// 逐行: sscanf(line, "DIR,%d", &state)
```

所以 Python 侧已支持输出 `DIR,<state>` 文本行，**SNN 侧 C++ 只需改脚本路径**：

| 项 | 约定 |
|---|---|
| 行格式 | `DIR,-1` / `DIR,0` / `DIR,1` / `DIR,2`（左/中/右/丢失） |
| 节拍 | 每 20ms 一行（`--period` 可调） |
| 语义 | 最新值覆盖旧值（atomic 变量天然如此） |
| 依赖 | `--loopback --stdout` 模式**零第三方依赖**；真机加 pyserial |

启动参数建议：

```bat
:: 无相机长跑测试（持续输出，不退出）
python -u v1_to_spikes.py --loopback --stdout --repeat

:: 真机
python -u v1_to_spikes.py COMx --stdout --repeat

:: 只输出 DIR 行（不打印调试行）
python -u v1_to_spikes.py --loopback --stdout --repeat --quiet
```

注意：非 DIR 行（调试输出）会被 `sscanf` 忽略，不影响解析；`--quiet` 可让输出更干净。

## 二、必须对齐的三件事

| 项 | 约定 | 不对齐的后果 |
|---|---|---|
| **端口** | 输入侧固定 **5566**；v1_to_spikes 用 `PUB bind tcp://*:5566`，SNN 侧 `SUB connect 127.0.0.1:5566` | 收不到任何包 |
| **topic** | `/robot/input_spikes` | 订阅不到（SUB 按前缀过滤） |
| **仿真步长** | v1_to_spikes 默认 `--snn-dt 0.0001`（0.1ms）；如果你的 Simulation 用 0.2ms，启动时改 `--snn-dt 0.0002` | 事件时间步数整体差 2 倍，脉冲落在错误时刻 |

神经元编号约定：**0 = 左（N0）、1 = 中（N1）、2 = 右（N2）**，其它 id 不会出现。

## 三、启动顺序

1. 先起 `v1_to_spikes.py`（它 bind 5566）
2. 再起 SNN 程序（connect 5566）

ZMQ PUB/SUB 有 slow joiner：订阅建立前发出的包会丢。真机持续出包时只影响最开始几个窗，无实际影响。

## 四、视觉线发送端怎么跑

### Python 版（主线，脚本改动灵活）

```bash
pip install pyserial pyzmq

# 真机（相机插 COM4）
python v1_to_spikes.py COM4

# 无相机自测：10 帧合成数据，20ms 一窗，自带 ZMQ 回读校验
python v1_to_spikes.py --loopback

# 只打印、不连 ZMQ
python v1_to_spikes.py COM4 --stdout

# 对齐你的仿真步长
python v1_to_spikes.py COM4 --snn-dt 0.0002
```

### 你这边：C++ 接收端示例

`spike_rx_demo.exe` 就是给你的验证工具，解析逻辑与 SNN 驱动完全一致：

```bat
spike_rx_demo.exe
```

期望输出：

```
batch#1   hdr_step=1    spikes=2  | 步50(0) 步150(0) | 事件步 50..150
batch#4   hdr_step=4    spikes=1  | 步700(1) | 事件步 700..700
batch#3   hdr_step=3    spikes=0 (空批)
```

源码 `spike_rx_demo.cpp`（单文件，`build_rx.cmd` 可重编），可直接抄进你的 SNN 工程。

### 备用：C++ 版发送端

`v1_to_spikes_bridge.exe` 与 Python 版功能等价（不需要 Python 环境），但主线仍是 Python 发送 + C++ 接收。

## 五、你这边的验证

不想改 C++ 也能先验证链路：`snn_rx_sim.py` 复现了 `ZMQAsyncInputOutputSpikeDriver` 的收包与步数换算。

```bash
python snn_rx_sim.py --dt 0.0001
```

期望输出（20ms 节拍、事件步单调递增）：

```
batch#3   hdr_step=4    spikes=1  | 事件步 700..700 | 仿真当前步 396 | 领先 +304 步
    neuron=1  time=0.070000s bts=0.000100s → step=700
done: 9 batches, 6 spikes, 0 bad
verdict: PASS
```

## 五·B、SNN 侧现成接收端（10/24 已验证，CPU 可跑）

`mainprogram/zmq_link_check.cpp`：最小 CPU 接收端（3 输入神经元→1 LIF，实时配速，4s 窗，输出落 `tutorial_output/zmq_link_check/`）。构建（新目录 build_zmq，不影响 build_tutorial）：

```powershell
cmake -S D:\snnplatform\net_package-main -B D:\snnplatform\net_package-main\build_zmq -G "Visual Studio 18 2026" -A x64 -DSNN_COMPUTE_BACKEND=CPU -DSNN_BUILD_ALL_MAINS=OFF -DSNN_DEFAULT_MAIN=zmq_link_check -DSNN_ENABLE_ZMQ=ON -DSNN_ENABLE_PINOCCHIO=OFF -DSNN_ZMQ_INCLUDE_DIR="D:/dev/zeromq/zeromq-4.3.5/include" -DSNN_ZMQ_LIBRARY="D:/dev/zeromq/build/lib/Release/libzmq-v145-mt-4_3_5.lib" -DSNN_BOOST_INCLUDE_DIR="C:/dev/boost_1_91_0"
cmake --build D:\snnplatform\net_package-main\build_zmq --config Release --target zmq_link_check
```

（zmq.hpp 需与 zmq.h 同目录，已从 `D:\dev\zeromq\` 拷入 `zeromq-4.3.5\include\`；运行需 `libzmq-v145-mt-4_3_5.dll` 在 exe 旁或 PATH。）

**对接必须知道的三条（10/24 实测）**：

1. **收实时事件必须 `RunSimulationRealtime`**（先 `EnableRealtime(10, 0.1f, 0.85f, 0.98f, 0.99f)`，参数与 `main_program.cpp` 一致）。`RunSimulationStep` 是 CPU 全速，几百步毫秒级跑完——视觉侧 20ms/窗的事件几乎全部落在仿真结束后，插入 EventQueue 也不执行（过期静默丢弃）。这就是"事件不执行/时间错位"的根源。
2. **`ZMQcommunication_test.cpp` 在 CPU 构建下跑不起来**：其 LIF 模型 `TimeDrivenLIF_Exponential_triple_GPU` 是 GPU 专用；CPU 用 `TimeDrivenLIF_Exponential_Decay`（教程同款）。
3. **自建 main 的两个隐性段错误点**：LIF 层要给 `NeuronParameter["V_th"]`（如 -55.0f）、连接要给 `SynapseModel = {ModelDescription{}}`——教程都带，漏了就在 `Simulation` 构造里崩。

验证 SNN 输出（5567）可用任意 ZMQ SUB 监听器（topic `/snn/output_spikes`）（线上 magic=0x53504B31，注意不是按 "SPK1" 字面的小端值）。

## 六、线格式（要自己解析时看）


每条消息三帧 multipart（空批只有前两帧）：

| 帧 | 长度 | 内容 |
|---|---|---|
| 1 | 21 B | topic 字符串 `/robot/input_spikes` |
| 2 | 16 B | `SPK1`(u32) + 版本 1(u32) + 批号(u32) + 脉冲数(u32) |
| 3 | n×12 B | 每个脉冲：`i32 neuron, f32 time(秒，绝对仿真时间), f32 basetimestep(秒/步)` |

小端序。SNN 侧换算：`Time = (int)(time / basetimestep)`，得到绝对步数，插入 EventQueue。

## 七、已知边界

- 输出节拍固定 20ms（`--period` 可调），串口读线程只保留最新帧，旧帧直接丢弃
- 事件时间以“首个 V1 包到达”为 0 点；SNN 从 0 步开始仿真，两边需同时启动才严格对齐
- 真机联调前建议先跑 `--loopback`（无需相机）确认 ZMQ 通，再上相机
