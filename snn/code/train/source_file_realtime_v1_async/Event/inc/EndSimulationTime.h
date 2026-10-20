/*
* 文件名：EndSimulationTime.h
* 定义了一个结束事件，在队列中标志着仿真结束，继承自Event类
*/
#ifndef ENDSIMULATIONTIME_H
#define ENDSIMULATIONTIME_H

#include "../source_file_realtime_v1_async/Event/inc/Event.h"
class Simulation;

class EndSimulationTime : public Event {
	public:


	    /*
	    * 无参构造函数
	    */
	    EndSimulationTime();

	    /*
	    * 有参构造函数
	    * time: 缁撴潫鏃堕棿
	    * index: 浜嬩欢闃熷垪绱㈠紩
	    */
	    EndSimulationTime(int time, int index);

	    /*
	    * 鏋愭瀯鍑芥暟
	    */
	    ~EndSimulationTime();

	    /*
	    * 浜嬩欢澶勭悊鍑芥暟
	    */
	    virtual void ProcessEvent(Simulation* simulation);

		/*
		* 鑾峰彇浜嬩欢鐨勪紭鍏堢骇
		*/
		virtual enum EventPriority getPriority();


};


#endif
