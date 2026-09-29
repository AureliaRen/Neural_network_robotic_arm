"""
非实时运动指令执行信息

ID: 路径ID, 对应调用moveAppend()时第二个参数; 类型string
ReachTarget: 轨迹是否到达目标点; 类型bool
WaypointIndex: 当前正在执行的轨迹目标点下标, 从0开始; 类型int
Error: 错误码, 运动指令执行前或执行中的错误; 类型error_code
Remark: 其它执行信息，目前包括目标点距离过近的告警信息; 类型string
"""
from __future__ import annotations
__all__ = ['Error', 'ID', 'ReachTarget', 'Remark', 'WaypointIndex']
Error: str = 'error'
ID: str = 'cmdID'
ReachTarget: str = 'reachTarget'
Remark: str = 'remark'
WaypointIndex: str = 'wayPointIndex'
