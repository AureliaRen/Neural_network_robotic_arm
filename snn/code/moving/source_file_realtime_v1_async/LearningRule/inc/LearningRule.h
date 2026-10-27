/*
* 文件名: LearningRule.h
* 定义了学习规则的基类，类似于NeuronModel类，我们将同类的学习规则放在同一类中统一管理
*/
#ifndef LEARNINGRULE_H
#define LEARNINGRULE_H

class SynapseState;
class Interconnections;
class Simulation;
class Neuron;
#include <map>
#include <string>
#include "boost/any.hpp"

class LearningRule {
	public:
		//学习规则的全局索引
		int LearningRuleID;

		//连接状态对象
		SynapseState* State;

		//State计数器
		int StateCounter;

		//状态初始化
		virtual void InitState(int NumberOfConnections, int NumberOfNeuron, float basetimestep) = 0;

		/*
		* 构造函数
		*/
		LearningRule();

		/*
		* 析构函数
		*/
		virtual ~LearningRule();

		/*
		* 当连接前脉冲到达时，调用该函数
		*/
		virtual void ApplyPreSynaticSpike(Interconnections* connection, int SpikeTime, Simulation* simulation) = 0;

		/*
		* 当连接后脉冲到达时，调用该函数
		*/
		virtual void ApplyPostSynaticSpike(Neuron* neuron, int SpikeTime, Simulation* sim) = 0;

		/*
		* 返回是否权重变化与突触后活动相关
		*/
		virtual bool ImplementPostSynaptic() = 0;

		/*
		* 返回是否权重变化与外加示教信息相关
		*/
		virtual bool ImplementTriggerSynaptic() = 0;

		/*
	   * 获取模型参数
	   */
		virtual std::map<std::string, boost::any> GetParameters() = 0;


};


#endif // LEARNINGRULE_H

