/*
* 文件名：InputCurrent.h
* 定义了外界输入的电流传入事件，继承自InputEvent
*/
#ifndef INPUTCURRENT_H
#define INPUTCURRENT_H

#include "../source_file_realtime_v1_async/Event/inc/Current/Current.h"
class InputCurrent : public Current {
	public:
		/*
		* 默认构造函数
		*/
		InputCurrent();

		/*
		* 构造函数
		* sourceNeuron：源神经元
		* time：时间
		* QueueIndex：队列索引
		* current：电流
		*/
		InputCurrent(Neuron* sourceNeuron, int time, int QueueIndex, float current);

		/*
		* 析构函数
		*/
		~InputCurrent();

		/*
		* 澶勭悊浜嬩欢
		* simulation：仿真对象
		*/
		virtual void ProcessEvent(Simulation* simulation);

		/*
		* 获取事件优先级
		*/
		virtual enum EventPriority getPriority();

};


#endif
