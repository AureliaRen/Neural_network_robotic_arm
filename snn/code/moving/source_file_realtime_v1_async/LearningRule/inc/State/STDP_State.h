/*
* 文件名： STDP_State.h
* 定义了STDP状态类，继承自SynapseState基类
*/

#ifndef STDP_STATE_H
#define STDP_STATE_H

#include "../source_file_realtime_v1_async/LearningRule/inc/SynapseState.h"
class STDP_State : public SynapseState {
	public:
		float LTP_tau;
		float LTD_tau;
		float inv_LTP_tau;
		float inv_LTD_tau;

		/*
		* 构造函数
		*/
		STDP_State(int NumConnections, float LTP_tau, float LTD_tau);

		/*
		* R_STDP对应的构造函数
		*/
		STDP_State(int NumConnections, float LTP_tau, float LTD_tau, bool R_STDP);

		/*
		* 析构函数
		*/
		virtual ~STDP_State();

		/*
		* 设置更新时间,并更新Apre与Apost
		*/
		virtual void SetUpdate(int ConnectionIndex, int Time, float Basetimestep);

		/*
		* 虚函数，应用突触前脉冲
		*/
		virtual void ApplyPresynapticSpike(int index);

		/*
		* 虚函数，应用突触后脉冲
		*/
		virtual void ApplyPostsynapticSpike(int index);

};

#endif
