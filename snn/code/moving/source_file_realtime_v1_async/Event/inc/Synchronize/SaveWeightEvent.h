/*
* 文件名：SaveWeightEvent.h
* 定义了保存权重事件，用于在训练过程中保存权重
*/

#ifndef SAVEWEIGTHEVENT_H
#define SAVEWEIGTHEVENT_H

#include "../source_file_realtime_v1_async/Event/inc/Event.h"
class Simulation;

class SaveWeightEvent : public Event {
	public:

		/*
		* 构造函数
		*/
		SaveWeightEvent(int newtime, Simulation* simulation);

		/*
		* 鏋愭瀯鍑芥暟
		*/
		~SaveWeightEvent();

		/*
		* 澶勭悊浜嬩欢鍑芥暟
		*/
		void ProcessEvent(Simulation* simulation);

		/*
		* 获取事件优先级
		*/
		virtual enum EventPriority getPriority();
};

#endif