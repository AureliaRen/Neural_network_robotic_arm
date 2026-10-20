# SNN 与机械臂集成说明（handoff）

分支：`feat/arm-action-server`

这个分支把两部分放在一起，方便机械臂同学一次性拿走：

1. **机械臂动作接口**（`xcoresdk_python-v0.4.1.C118_0/arm_controller/`）
   - `action_server.py`：TCP 服务，接收 `left / right / stop`
   - `action_client.py`：命令行/测试客户端
   - `ARM_INTEGRATION.md`、`ARM_DRY_RUN_VERIFICATION.md`：协议、安全、验证记录

2. **SNN 复现代码快照**（`snn/`）
   - `snn/code/train`：静态 Fig.4 网络 + reward/punishment 训练
   - `snn/code/moving`：在线移动 cue（Fig.13 式）训练
   - `snn/analysis`：结果分析脚本
   - `snn/results`：逐 seed CSV、图、README、Fig.12 与 100/300 cycle 对比
   - `snn/runs`：本轮运行的原始输出（trajectory、weights、summary 等）

## 动作映射

| SNN 输出 | 含义 | 机械臂 wire | 关节 1 |
|---|---|---|---|
| `0` / `TURN_LEFT` | 向左 | `-1` / `left` | `+step_rad` |
| `1` / `TURN_RIGHT` | 向右 | `1` / `right` | `-step_rad` |
| `2` / `NO_DECISION` | 停 | `0` / `stop` | 不运动 |

TCP 默认 `9000`，一行一条命令；服务回 `OK ...` 或 `ERR ...`。

## 已在本机验证

- `action_server.py` 默认 dry-run，不连接真机。
- 合成动作 `0,1,2` -> `left,right,stop` 全部收到 `OK`。
- 真实 SNN 轨迹 `snn/runs/tracking_train_reward_910001/trajectory.csv`
  前 5 个 action 全部为 `0`，按 `-1` 发送并收到 `OK`。
- 没有连接真机、没有真实关节运动。

细节见 `xcoresdk_python-v0.4.1.C118_0/arm_controller/ARM_DRY_RUN_VERIFICATION.md`。

## 未验证 / 不在本分支

- 真机关节方向、限位、速度、急停行为。
- `stop` 是否能中断已经在执行的 `MoveAbsJ`（当前是“本次不动作”）。
- 视觉模块（另一项工作，尚未统一格式）。
- C++ SNN 主程序里的 TCP sink；当前先用 `action_client.send_action` /
  独立 Python 桥接验证，C++ 决策循环还没有直接发 socket。
- 完整的视觉-SNN-机械臂闭环。

## 运行

### 机械臂侧（本机干跑）

```powershell
cd xcoresdk_python-v0.4.1.C118_0\arm_controller
python action_server.py --dry-run
python action_client.py --action left
python action_client.py --action right
python action_client.py --action stop
```

### 机械臂侧（真机，仅负责人）

```powershell
python action_server.py --real --home --step-rad 0.02 --speed 20
```

现场必须有负责人和急停；第一次只走一小步，先确认关节 1 正方向是不是“左”。

### SNN 侧

构建和运行命令、seed、哈希、失败记录都在 `snn/results/README.md`。
构建需要 MSVC（VS 2026）、CMake、Boost 头文件；构建产物
（`build_ninja`、exe/obj/lib）没有提交，需要在本机重新编译。

## 为什么放在同一分支

- 机械臂同学需要同时看到“接口怎么调”和“SNN 输出是什么”，
  分开两个仓库反而容易漏掉映射。
- 代码仍按目录隔离：`snn/` 不依赖 `xcoresdk`，`action_server.py`
  也不依赖 SNN；两边只通过 TCP 的 `left/right/stop` 耦合。
- 本分支的提交历史：
  1. `a2521ed` 机械臂 joint-1 动作服务；
  2. 本次提交：SNN 复现快照 + 本说明。

## 结果边界

SNN 侧是 **条件性行为复现**，不是论文数值的逐点复现。主要 caveat：
reward 训练在 300 cycles 时大量权重饱和到 `[0,2]`；100 cycles 可避免饱和、
更接近论文 Fig.12 的量级，但行为更弱；seed 730003 在 100/200 cycles
没有 reward，300 cycles 才学习。详见 `snn/results/README.md` 与
`snn/results/pilot100.md`。
