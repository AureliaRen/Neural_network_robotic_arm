# 机械臂动作接口（左 / 右 / 停）

`action_server.py` 是给 SNN/上层控制用的 **关节 1** 动作服务，语义与
`simple.py` / `bionic_arm_controller.py` 一致：

| 输入 | 含义 | 关节 1 变化 |
|---|---|---|
| `left` / `-1` | 向左 | `+step_rad` |
| `right` / `1` | 向右 | `-step_rad` |
| `stop` / `0` | 停 | 不运动 |

注意：
- 原来的 `servicer.py` 是 **末端绕 z 轴 ±8°** 的控制器，并且把 `-1`
  当成非法值；它和本服务不是同一套动作语义。不要同时占用 9000 端口。
- 本服务默认 **dry-run**，只打印和回包，不连接机械臂。真机必须显式加
  `--real`，并且由实验室机械臂负责人现场确认、手边有急停。
- `stop` 是“本次不动作”，不会中断已经在执行的 `MoveAbsJ`。急停请用
  示教器/急停按钮；后续可再加独立的异步 stop 通道。

## 协议

TCP，UTF-8，一行一条命令，回车换行结束：

```
-1        # 或 left
0         # 或 stop
1         # 或 right
```

服务回一行：`OK ...` 或 `ERR ...`。

## 运行

在 `arm_controller/` 目录下：

```powershell
# 1) 干跑，不连机械臂
python action_server.py --dry-run

# 2) 另一个终端发命令
python action_client.py --action left
python action_client.py --action right
python action_client.py --action stop
```

真机（仅在负责人同意后）：

```powershell
# 先小步、低速；--home 会先回 init.pos_init 的固定姿态
python action_server.py --real --home --step-rad 0.02 --speed 20
```

参数/环境变量：
- `--step-rad`，默认 `0.1` rad；首次真机建议 `0.02`。
- `--speed`，默认 `50`；首次真机建议 `20`。
- `--port`，默认 `9000`。
- `ARM_IP`，默认 `192.168.0.160`。

## SNN 对接

SNN 侧只需要把三种决策映射成一行：

```
left  -> -1
right ->  1
stop  ->  0
```

Python 可以直接：

```python
from action_client import send_action
send_action("left", host="127.0.0.1", port=9000)
```

C++ 侧可以用 `Winsock`/`asio` 连 `127.0.0.1:9000`，每条命令以 `\n`
结尾，读回一行 ack 即可。当前 `fig4_closed_loop` 还没有 socket sink；
下一步是在决策循环里把 `TURN_LEFT/TURN_RIGHT/NO_DECISION` 映射后发送。

## 安全清单

- [ ] 机械臂负责人现场，手边急停。
- [ ] 工作空间内无人、无杂物。
- [ ] 先 dry-run 验证 left/right/stop 和 ack。
- [ ] 真机第一次只发一个小步（`--step-rad 0.02 --speed 20`）。
- [ ] 确认关节 1 正方向确实是“左”；如果方向反了，改 `--step-rad`
      符号或在服务里交换 left/right。
- [ ] 不要用 `servicer.py` 和 `action_server.py` 同时监听 9000。
