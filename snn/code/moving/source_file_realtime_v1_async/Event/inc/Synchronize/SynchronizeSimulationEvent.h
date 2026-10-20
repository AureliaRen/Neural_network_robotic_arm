/*
* 文件名：SynchronizeSimulationEvent.h
* 定义了同步事件类SynchronizeSimulationEvent，用于同步多个线程的仿真进程
*/

#ifndef SYNCHRONIZESIMULATIONEVENT_H
#define SYNCHRONIZESIMULATIONEVENT_H

#include "../source_file_realtime_v1_async/Event/inc/Event.h"
class Simulation;

class SynchronizeSimulationEvent : public Event {
	public:

		/*
		* 构造函数
		* newtime: 事件发生的时间
		* QueueIndex: 事件队列的索引
		*/
		SynchronizeSimulationEvent(int newtime, int QueueIndex);

		/*
		* 鏋愭瀯鍑芥暟
		*/
		~SynchronizeSimulationEvent();

		/*
		* 铏氬嚱鏁帮紝鐢ㄤ簬澶勭悊浜嬩欢
		* Simulation: 浠跨湡瀵硅薄
		*/
		virtual void ProcessEvent(Simulation* simulation);

		/*
		* 获取事件优先级
		*/
		virtual enum EventPriority getPriority();
	    

};

#endif
