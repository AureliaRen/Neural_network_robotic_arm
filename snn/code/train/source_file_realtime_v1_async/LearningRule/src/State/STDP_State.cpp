#include "../source_file_realtime_v1_async/LearningRule/inc/State/STDP_State.h"
#include <iostream>

STDP_State::STDP_State(int NumConnections, float LTP_tau, float LTD_tau) : SynapseState(NumConnections, 2), LTP_tau(LTP_tau), LTD_tau(LTD_tau) {
	this->inv_LTP_tau = 1.0 / LTP_tau;
    this->inv_LTD_tau = 1.0 / LTD_tau;
}

STDP_State::STDP_State(int NumConnections, float LTP_tau, float LTD_tau, bool R_STDP) : SynapseState(NumConnections, 3), LTP_tau(LTP_tau), LTD_tau(LTD_tau) {
	this->inv_LTP_tau = 1.0 / LTP_tau;
	this->inv_LTD_tau = 1.0 / LTD_tau;
}

STDP_State::~STDP_State() {}

void STDP_State::SetUpdate(int ConnectionIndex, int Time, float Basetimestep) {
	//计算衰减时间
	float deltaTime = (Time - this->LastUpdate[ConnectionIndex]) * Basetimestep;
	//计算LTP的衰减系数
	this->StateDecay(ConnectionIndex, 0, exp(-deltaTime * this->inv_LTP_tau));
	//计算LTD的衰减系数
	this->StateDecay(ConnectionIndex, 1, exp(-deltaTime * this->inv_LTD_tau));
	//更新最后更新时间
    this->LastUpdate[ConnectionIndex] = Time;
}

void STDP_State::ApplyPresynapticSpike(int index) {
	this->StateJump(index, 0, 1.0);
}

void STDP_State::ApplyPostsynapticSpike(int index) {
	this->StateJump(index, 1, 1.0);
}
