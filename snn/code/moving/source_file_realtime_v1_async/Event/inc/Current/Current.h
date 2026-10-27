/*
* 文件名：Current.h
* 定义了电流事件基类
*/

#ifndef CURRENT_H
#define CURRENT_H

#include "../source_file_realtime_v1_async/Event/inc/Event.h"
#include "../source_file_realtime_v1_async/Neuron/inc/Neuron.h"

class Current : public Event {
	public:
		//鐢垫祦婧愮缁忓厓
		Neuron* SourceNeuron;

		//电流大小
		float current;

		/*
		* 默认构造函数
		*/
		Current();

		/*
		* 构造函数
		*/
		Current(Neuron* sourceNeuron, int time, int QueueIndex, float current);

		/*
		* 析构函数
		*/
        ~Current();

		/*
		* 澶勭悊浜嬩欢
		*/
		virtual void ProcessEvent(Simulation* simulation) = 0;

		/*
		* 查询事件优先级（纯虚函数）
		*/
		virtual enum EventPriority getPriority() = 0;

};


#endif

