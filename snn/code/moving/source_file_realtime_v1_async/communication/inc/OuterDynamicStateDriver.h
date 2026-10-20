/*
* 文件名: OuterDynamicStateDriver.h
* 定义外部动力学状态输出driver的基类
*/

#ifndef OUTERDYNAMICSTATEDRIVER_H
#define OUTERDYNAMICSTATEDRIVER_H

#include <array>

struct OuterDynamicJointState {
    std::array<double, 2> q;
    std::array<double, 2> qv;
    std::array<double, 2> q_des;
    std::array<double, 2> qv_des;
    std::array<double, 2> tau_total;
};

class OuterDynamicStateDriver {
public:
    virtual ~OuterDynamicStateDriver();

    virtual void WriteJointState(
        int time_step,
        float base_timestep_ms,
        const OuterDynamicJointState& state) = 0;

    virtual void FlushBuffers() = 0;
};

#endif
