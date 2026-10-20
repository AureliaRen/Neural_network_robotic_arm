/*
* 文件名：WithPreAndPostSynaptic.h
* 定义了带有后突触效应的突触可塑性规则,继承自LearningRule
*/

#ifndef WITHPREANDPOSTSYNAPTIC_H
#define WITHPREANDPOSTSYNAPTIC_H

#include "../source_file_realtime_v1_async/LearningRule/inc/LearningRule.h"
class Neuron;
class Simulation;
class WithTriggerAndPostSynaptic : public LearningRule {
public:

	/*
	* 状态初始化
	*/
	virtual void InitState(int NumberOfConnections, int NumberOfNeuron, float basetimestep) = 0;

	/*
	* 构造函数
	*/
	WithTriggerAndPostSynaptic();

	/*
	* 鏋愭瀯鍑芥暟
	*/
	virtual ~WithTriggerAndPostSynaptic();

	/*
	* 当连接前脉冲到达时，调用该函数
	*/
	virtual void ApplyPreSynaticSpike(Interconnections* connection, int SpikeTime, Simulation* simulation) = 0;

	/*
	* 当连接后脉冲到达时，调用该函数
	*/
	virtual void ApplyPostSynaticSpike(Neuron* neuron, int SpikeTime, Simulation* simulation) = 0;

	/*
	* 杩斿洖鏄惁鏉冮噸鍙樺寲涓庣獊瑙﹀悗娲诲姩鐩稿叧
	*/
	virtual bool ImplementPostSynaptic();

	/*
	* 杩斿洖鏄惁鏉冮噸鍙樺寲涓庣獊瑙﹀墠娲诲姩鐩稿叧
	*/
	virtual bool ImplementTriggerSynaptic();

};




#endif // WITHPREANDPOSTSYNAPTIC_H
