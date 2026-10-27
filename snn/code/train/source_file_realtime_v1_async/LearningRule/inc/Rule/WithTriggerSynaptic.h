/*
* 文件名：WithTriggerSynaptic.h
* 定义了一个触发式学习规则类，继承自LearningRule，通过外界的触发信号来改变突触权重
*/
#ifndef WITHTRIGGERSYNAPTIC_H
#define WITHTRIGGERSYNAPTIC_H

#include "../source_file_realtime_v1_async/LearningRule/inc/LearningRule.h"

class WithTriggerSynaptic : public LearningRule {
	public:
		/*
		* 构造函数
		*/
		WithTriggerSynaptic();

		/*
		* 鏋愭瀯鍑芥暟
		*/
		~WithTriggerSynaptic();

		/*
		* 状态初始化
		*/
		virtual void InitState(int NumberOfConnections, int NumberOfNeuron, float basetimestep) = 0;

		/*
		* 当连接前脉冲到达时，调用该函数
		*/
		virtual void ApplyPreSynaticSpike(Interconnections* connection, int SpikeTime, Simulation* simulation) = 0;

		/*
		* 当连接后脉冲到达时，调用该函数
		*/
		virtual void ApplyPostSynaticSpike(Neuron* neuron, int SpikeTime, Simulation* simulation) {};

		/*
		* 杩斿洖鏄惁鏉冮噸鍙樺寲涓庣獊瑙﹀悗娲诲姩鐩稿叧
		*/
		virtual bool ImplementPostSynaptic();

		/*
		* 杩斿洖鏄惁鏉冮噸鍙樺寲涓庣獊瑙﹀墠娲诲姩鐩稿叧
		*/
		virtual bool ImplementTriggerSynaptic();

		/*
	   * 鑾峰彇妯″瀷鍙傛暟
	   */
		virtual std::map<std::string, boost::any> GetParameters() = 0;

};






#endif // !WITHTRIGGERSYNAPTIC_H
