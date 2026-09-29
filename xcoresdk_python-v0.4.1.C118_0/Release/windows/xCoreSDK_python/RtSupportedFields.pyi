"""
机器人实时数据支持的字段

jointPos_m: 关节角度 [rad] - ArrayXD
tcpPose_m: 末端位姿, 相对于基坐标系, 行优先齐次变换矩阵 - Array16D
tcpPoseAbc_m: 末端位姿, 相对于基坐标系 [X,Y,Z,Rx,Ry,Rz] - Array6D
elbow_m: 臂角 [rad] - double
keypads: 末端按键状态 - ArrayXD
"""
from __future__ import annotations
__all__ = ['elbow_m', 'jointPos_m', 'keypads', 'tcpPoseAbc_m', 'tcpPose_m']
elbow_m: str = 'psi_m'
jointPos_m: str = 'q_m'
keypads: str = 'io_keypad'
tcpPoseAbc_m: str = 'pos_abc_m'
tcpPose_m: str = 'pos_m'
