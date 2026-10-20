# SNN + 机械臂集成分支说明

分支：`feat/arm-action-server`

这个分支同时包含两部分，但两者保持独立：

1. **机械臂侧（本仓库原有 SDK + 新增客户端）**
   - `xcoresdk_python-v0.4.1.C118_0/arm_controller/action_client.py`
   - `xcoresdk_python-v0.4.1.C118_0/arm_controller/ARM_INTEGRATION.md`
   - 服务端由机械臂同学维护的 `servicer.py` 提供；本仓库不再包含参考服务端。

2. **SNN 侧（`snn/`，Fig.4 DA-STDP 复现快照）**
   - `snn/code/train`：静态两单元网络，reward / punishment。
   - `snn/code/moving`：在线移动线索（Fig.13 式）训练与评估。
   - `snn/results`：逐种子 CSV、图、结果说明。
   - `snn/runs`：原始运行输出。
   - `snn/analysis`、`snn/docs`：分析脚本和运行脚本。
   - 不包含 `build_ninja` 编译产物；需要在本机重新构建。

## 动作接口

固定为三种结果：

| SNN 动作 | SNN 数值 | 线上值 | 含义 |
|---|---:|---:|---|
| `TURN_LEFT` | 0 | `-1` | 左转 |
| `TURN_RIGHT` | 1 | `1` | 右转 |
| `NO_DECISION` | 2 | `0` | 不转 |

TCP 一行一条，UTF-8，`\n` 结尾；服务端不回包。客户端发送后立即返回，不等待 ack。

## 已验证 / 未验证

已验证：
- `action_client.py` 可单次发送和交互式连续发送，发送后不阻塞。
- 之前的 dry-run 参考服务端测试（`action_server.py`）已经随该文件删除；当前客户端只面向机械臂同学的 `servicer.py`。
- SNN 侧结果和构建说明在 `snn/results/README.md`。

未验证：
- 真机关节方向、限位、速度、`stop` 是否能中断运动。
- 机械臂同学的 `servicer.py` 是否把 `-1/0/1` 正确映射成左/右/不转。
- 视觉输入。
- C++ SNN 决策循环到 TCP 的自动 sink（目前只有客户端和映射，未把 `fig4_closed_loop` 直接接到 socket）。
- 完整闭环实时性。

## 运行

机械臂客户端：

```powershell
cd xcoresdk_python-v0.4.1.C118_0\arm_controller
python action_client.py --action stop --host 172.30.187.22 --port 9000
python action_client.py --interactive --host 172.30.187.22 --port 9000
```

SNN 构建与运行：见 `snn/results/README.md`。需要 Windows + MSVC（VS 2026）、
CMake、Ninja、Boost 头文件；先 `vcvars64.bat`，再 `cmake -G Ninja`。
`build_ninja` 未提交。

## 结果边界

`fig4_reproduction` 是**条件性行为复现**，不是论文数值的精确复现。主要 caveat：

- 300 cycle reward 训练行为强，但大量权重饱和到 `[0,2]`；100 cycle 去掉饱和、
  Fig.12 幅度更接近论文（约 0.1-0.2），但行为较弱。
- seed 730003 在 100/200 cycle 没有 reward，只有 300 cycle 才学习；其移动 triangle
  反而比未训练更差。
- 论文没有给收敛停止规则、`w_max`、噪声刷新规则等关键细节，固定 100/300 cycle
  都是协议假设。
- 视觉尚未接入；当前只完成机械臂命令侧。

## 提交说明

- `a2521ed`：机械臂动作服务/客户端/文档（服务端后来删除，客户端保留并改为不等回包）。
- `acb14fd`：完整 SNN 复现快照（`snn/`，1074 文件，约 11.26 MB）。
- 本次提交：客户端改为 fire-and-forget，固定 `left=-1 / right=1 / stop=0`，
  更新文档，删除过时的参考服务端验证文档。
