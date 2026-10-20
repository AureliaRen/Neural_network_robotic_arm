/*
* 文件名：InputSpike.h
* 用于处理外界输入的Spike事件，继承自Spike类
*/

#ifndef INPUTSPIKE_H
#define INPUTSPIKE_H
#include "../source_file_realtime_v1_async/Event/inc/Spike/Spike.h"
#include "../source_file_realtime_v1_async/Event/inc/Event.h"

class InputSpike : public Spike {	
	public:

		/*
		* 默认构造函数
		*/
		InputSpike();

		/*
		* 带参构造函数
		* neuron: 鍙戝嚭Spike鐨勭缁忓厓
		* time: 浜嬩欢鏃堕棿
		* QueueIndex: 鍙戝嚭Spike鐨凲ueueIndex
		*/
		InputSpike(Neuron* neuron, int time, int QueueIndex);

		/*
		* 鏋愭瀯鍑芥暟
		*/
		~InputSpike();

		/*
		* 虚函数，处理该事件
		*/
		virtual void ProcessEvent(Simulation* simulation);

		virtual void ProcessEvent(Simulation* simulation, RealTimeRestrictionLevel level);

		/*
		* 查询事件优先级
		*/
		virtual enum EventPriority getPriority();

};

#endif
